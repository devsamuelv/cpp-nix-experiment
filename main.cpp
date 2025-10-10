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
#include <rclcpp/rclcpp.hpp>
#include "image_transport/camera_publisher.h"

using namespace std::chrono_literals;

// Heap declared array
cv::Vec3b *colormap = new cv::Vec3b[57]{
    cv::Vec3b(128, 64, 128),
    cv::Vec3b(244, 35, 232),
    cv::Vec3b(70, 70, 70),
    cv::Vec3b(102, 102, 156),
    cv::Vec3b(190, 153, 153),
    cv::Vec3b(153, 153, 153),
    cv::Vec3b(250, 170, 30),
    cv::Vec3b(220, 220, 0),
    cv::Vec3b(107, 142, 35),
    cv::Vec3b(152, 251, 152),
    cv::Vec3b(0, 130, 180),
    cv::Vec3b(220, 20, 60),
    cv::Vec3b(255, 0, 0),
    cv::Vec3b(0, 0, 142),
    cv::Vec3b(0, 0, 70),
    cv::Vec3b(0, 60, 100),
    cv::Vec3b(0, 80, 100),
    cv::Vec3b(0, 0, 230),
    cv::Vec3b(119, 11, 32),
};

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
    *buffer = std::vector<uchar>();
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
  tensor = tensor.to(at::kByte).to(torch::kCPU);

  return cv::Mat(cv::Size(tensor.size(1), tensor.size(0)), CV_8UC1, tensor.mutable_data_ptr<uchar>());
};

cv::Mat *applyFilter(cv::Mat *filter)
{
  // Mat size x, y
  cv::Size size(2048, 1024);

  // Heap mat
  cv::Mat *colorMat = new cv::Mat(size, CV_8UC3);

  for (int x = 0; x < (*filter).rows; x++)
  {
    cv::Mat mat = filter->row(x);

    for (int y = 0; y < mat.cols; y++)
    {
      uchar c = (*filter).at<uchar>(x, y);
      cv::Vec3b point = colormap[c];

      (*colorMat->ptr<cv::Vec3b>(x, y)) = point;
    }
  }

  return colorMat;
}

void camera_thread(std::reference_wrapper<cv::VideoCapture> cap, std::reference_wrapper<VideoManager> manager)
{
  // I think the model needs to be transfered to torchscript. I don't want to write a c++ version.
  torch::jit::Module model;

  const std::string path = "/home/samuel/Documents/code/robotics/teleop-control/"
                           "model.pt";
  model = torch::jit::load(path);

  // Enable pytorch inference
  torch::InferenceMode(true);
  at::Device device = at::kCPU;
  cv::Mat frame;

  model.eval();
  model.to(device);

  while (cap.get().isOpened())
  {
    cap.get().read(frame);

    // Convert the color scheme and channels to a torch transferable format (CV_32FC3) 32-bit data with 3 channels.
    frame.convertTo(frame, CV_32FC3, 1.0 / 255.0);
    at::Tensor image_tensor = torch::from_blob(frame.data, {frame.rows, frame.cols, frame.channels()}, at::kFloat).to(device);
    image_tensor = image_tensor.permute({2, 0, 1});

    // Normalize (ImageNet)
    image_tensor = torch::data::transforms::Normalize<>(
        {0.485, 0.456, 0.406},
        {0.229, 0.224, 0.225})(image_tensor);

    image_tensor = image_tensor.unsqueeze(0); // Batch dimention
    image_tensor = image_tensor.to(at::kFloat);

    auto output = model.forward({image_tensor}).toTuple();
    auto mask_tuple = output.get();

    // H, W, 1 Image
    auto pred = torch::argmax(mask_tuple->elements()[0].toTensor(), 1).squeeze(0).data();
    auto out = torchTensortoCVMat(pred);

    cv::Mat *colorMat = applyFilter(&out);

    manager.get()
        .update_buffer(*colorMat);
    frame.release();
  }

  delete &frame;
  // Disable pytorch inference on exit.
  torch::InferenceMode(false);
}

size_t get_size(std::shared_ptr<std::vector<uchar>> a)
{
  return (*a).size();
}

int main(int argc, char *argv[])
{
  static cv::VideoCapture capture("/dev/video0", cv::CAP_V4L2);
  static VideoManager manager;

  capture.set(cv::CAP_PROP_EXPOSURE, 0);
  capture.set(cv::CAP_PROP_FRAME_WIDTH, 1024);
  capture.set(cv::CAP_PROP_FRAME_HEIGHT, 2048);
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

  rclcpp::init(argc, argv);

  svr.listen("0.0.0.0", 8080);
  cameraThread.detach();
}