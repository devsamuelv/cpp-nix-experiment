#include <iostream>
#include <vector>
#include <opencv4/opencv2/opencv.hpp>
#include <string>
#include <httplib.h>
#include <algorithm>
#include <format>
#include <mutex>
#include <thread>
#include <torch/library.h>
#include <torch/torch.h>
#include <torch/script.h>
#include <torch/types.h>
#include <ATen/ATen.h>
#include "src/image/util.h"

class VideoManager
{
private:
  std::mutex buffer_mutex;
  std::shared_ptr<std::vector<uchar>> buffer = std::make_shared<std::vector<uchar>>();

public:
  void
  update_buffer(cv::Mat mat)
  {
    std::unique_lock<std::mutex> lock(buffer_mutex);
    cv::imencode(".jpg", mat, *buffer);
    lock.unlock();
  };

  std::vector<uchar> get_buffer()
  {
    std::lock_guard lock(buffer_mutex);
    return *buffer.get();
  };
};

cv::Mat torchTensortoCVMat(torch::Tensor &tensor)
{
  tensor = tensor.squeeze().detach();
  // tensor = tensor.permute({1, 2}).contiguous();
  // tensor = tensor.to(torch::kU8);
  tensor = tensor.to(at::kByte);
  tensor = tensor.to(torch::kCPU);
  int64_t height = tensor.size(0);
  int64_t width = tensor.size(1);

  // Causes segfaults

  return cv::Mat(cv::Size(width, height), CV_8UC3, tensor.mutable_data_ptr<uchar>());
};

void camera_thread(std::reference_wrapper<cv::VideoCapture> cap, std::reference_wrapper<VideoManager> manager)
{
  // I think the model needs to be transfered to torchscript. I don't want to write a c++ version.
  torch::jit::Module model;

  const std::string path = "/home/samuel/Documents/code/robotics/teleop-control/"
                           "model.pt";
  model = torch::jit::load(path);

  // We use Aten namespace because the torch namespace usses autodiff which we disable using inference.

  // Enable pytorch inference
  torch::InferenceMode(true);

  model.eval();

  while (cap.get().isOpened())
  {
    cv::Mat frame;
    cap.get().read(frame);

    // std::vector<torch::jit::IValue> inputs;

    frame.convertTo(frame, CV_32FC3, 1.0 / 255.0);
    at::Tensor image_tensor = torch::from_blob(frame.data, {frame.rows, frame.cols, frame.channels()}, at::kFloat);
    image_tensor = image_tensor.permute({2, 0, 1});

    // Optionally, clone to own memory
    image_tensor = image_tensor.clone();

    // Normalize (ImageNet)
    image_tensor = torch::data::transforms::Normalize<>(
        {0.485, 0.456, 0.406},
        {0.229, 0.224, 0.225})(image_tensor);

    image_tensor = image_tensor.unsqueeze(0).to(at::kCPU); // Batch dimention
    image_tensor = image_tensor.to(at::kFloat);

    // inputs.emplace_back(image_tensor);

    // This is practically pseudocode, it isn't correct.
    auto output = model.forward({image_tensor}).toTuple();
    auto mask_tuple = output.get();

    // H, W, 1 Image
    auto pred = torch::argmax(mask_tuple->elements()[0].toTensor(), 1).squeeze(0).to(at::kCPU).data();
    auto out = torchTensortoCVMat(pred);

    manager.get()
        .update_buffer(out);
  }

  // Disable pytorch inference on exit.
  torch::InferenceMode(false);
}

size_t get_size(std::shared_ptr<std::vector<uchar>> a)
{
  return (*a).size();
}

int main(int, char **)
{
  static cv::VideoCapture capture("/dev/video0", cv::CAP_V4L2);
  static VideoManager manager;

  capture.set(cv::CAP_PROP_EXPOSURE, 0);
  capture.set(cv::CAP_PROP_FRAME_WIDTH, 720);
  capture.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
  capture.set(cv::CAP_PROP_FPS, 30);

  std::thread cameraThread(
      camera_thread,
      std::ref(capture),
      std::ref(manager));

  // HTTP
  httplib::Server svr;

  svr.Get("/stream", [](const httplib::Request &, httplib::Response &res)
          { res.set_chunked_content_provider(
                "multipart/x-mixed-replace; boundary=frame", // Content type
                [&](size_t offset, httplib::DataSink &sink)
                {
                  std::vector<uchar> video_data = (manager.get_buffer());

                  std::string preheader(std::format("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: {}\r\n\r\n", video_data.size()));
                  std::string payload(video_data.begin(), video_data.end());
                  std::string end("\r\n");

                  sink.write(preheader.data(), preheader.size());
                  sink.write(payload.data(), payload.size());
                  sink.write(end.data(), end.size());
                  sink.os.flush();

                  return true; // return 'false' if you want to cancel the process.
                }); });

  std::cout << "Server running!" << std::endl;

  svr.listen("0.0.0.0", 8080);
  cameraThread.detach();
}