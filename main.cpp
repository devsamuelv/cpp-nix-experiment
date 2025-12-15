#include "image_transport/image_transport.hpp"
#include <ATen/ATen.h>
#include <algorithm>
#include <format>
#include <httplib.h>
#include <iostream>
#include <mutex>
#include <opencv4/opencv2/opencv.hpp>
#include <rclcpp/rclcpp.hpp>
#include <string>
#include <thread>
#include <torch/library.h>
#include <torch/script.h>
#include <torch/torch.h>
#include <torch/types.h>
#include <vector>
#include <cv_bridge/cv_bridge.h>
#include <chrono>
#include <csignal>
#include "src/video/video_manager.hpp"
#include "src/helpers/torch_helper.hpp"
#include "src/tasks/testTask.hpp"

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

static volatile sig_atomic_t sig_caught = 0;
static httplib::Server svr;

cv::Mat applyFilter(cv::Mat *image)
{
  cv::Size size((*image).cols, (*image).rows);

  // Mat is allocated on stack and is using RAII to deallocate
  // Heap allocated is terrible for this usecase since its inside a loop and we don't want a mem eater
  cv::Mat colorMat(size, CV_8UC3);

  for (int x = 0; x < (*image).rows; x++)
  {
    cv::Mat mat = image->row(x);

    for (int y = 0; y < mat.cols; y++)
    {
      uchar c = (*image).at<uchar>(x, y);
      cv::Vec3b point = colormap[c];

      auto color_mat_pointer = (&colorMat)->ptr<cv::Vec3b>(x, y);

      *color_mat_pointer = point;
    }
  }

  return colorMat;
}

void camera_thread(std::reference_wrapper<cv::VideoCapture> cap,
                   std::reference_wrapper<VideoManager> manager, std::reference_wrapper<rclcpp::Node::SharedPtr> node)
{
  // I think the model needs to be transfered to torchscript. I don't want to
  // write a c++ version.
  torch::jit::Module model;

  const std::string path =
      "/home/samuel/Documents/code/robotics/teleop-control/"
      "model.pt";
  model = torch::jit::load(path);

  // Enable pytorch inference
  torch::InferenceMode(true);
  at::Device device = at::kCPU;
  cv::Mat frame;

  // RCL

  image_transport::ImageTransport it{node.get()};
  static image_transport::Publisher pub = it.advertise("camera/image", 1);
  static image_transport::Publisher pub_raw = it.advertise("camera/image_raw", 1);

  auto now = std::chrono::system_clock::now();
  auto now_raw = std::chrono::system_clock::now();

  model.eval();
  model.to(device);

  while (cap.get().isOpened() && sig_caught == 0)
  {
    cap.get().read(frame);

    cv::MatSize inital_frame_size = frame.size;

    auto duration_raw = now_raw.time_since_epoch();
    auto duration_ms_raw = std::chrono::duration_cast<std::chrono::milliseconds>(duration_raw).count();

    if (rclcpp::ok() && duration_ms_raw > 150)
    {
      std_msgs::msg::Header hdr;
      sensor_msgs::msg::Image::SharedPtr msg;

      msg = cv_bridge::CvImage(hdr, "bgr8", frame).toImageMsg();

      pub_raw.publish(msg);

      rclcpp::spin_some(node);
      now_raw = std::chrono::system_clock::now();
    }

    // Convert the color scheme and channels to a torch transferable format
    // (CV_32FC3) 32-bit data with 3 channels.
    frame.convertTo(frame, CV_32FC3, 1.0 / 255.0);
    at::Tensor image_tensor =
        torch::from_blob(frame.data, {frame.rows, frame.cols, frame.channels()},
                         at::kFloat)
            .to(device);
    image_tensor = image_tensor.permute({2, 0, 1});

    // Normalize (ImageNet)
    image_tensor = torch::data::transforms::Normalize<>(
        {0.485, 0.456, 0.406}, {0.229, 0.224, 0.225})(image_tensor);

    image_tensor = image_tensor.unsqueeze(0); // Batch dimention
    image_tensor = image_tensor.to(at::kFloat);

    auto output = model.forward({image_tensor}).toTuple();
    auto mask_tuple = output.get();

    // H, W, 1 Image
    auto pred = torch::argmax(mask_tuple->elements()[0].toTensor(), 1)
                    .squeeze(0)
                    .data();
    auto out = TorchHelpers::torchTensortoCVMat(pred);

    cv::Mat colorMat = applyFilter(&out);

    auto duration = now.time_since_epoch();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    if (rclcpp::ok() && duration_ms > 150)
    {
      std_msgs::msg::Header hdr;
      sensor_msgs::msg::Image::SharedPtr msg;

      msg = cv_bridge::CvImage(hdr, "bgr8", colorMat).toImageMsg();

      pub.publish(msg);

      rclcpp::spin_some(node);
      now = std::chrono::system_clock::now();
    }

    manager.get().update_buffer(colorMat);
    frame.release();
  }

  // Disable pytorch inference on exit.
  torch::InferenceMode(false);
}

size_t get_size(std::shared_ptr<std::vector<uchar>> a) { return (*a).size(); }

void signal_handler(int signal)
{
  sig_caught = 1;

  svr.stop();

  std::cout << "PROGRAM TERMINATED!" << std::endl;
}

int main(int argc, char *argv[])
{
  static cv::VideoCapture capture("/dev/video4", cv::CAP_V4L2);
  static VideoManager manager;

  rclcpp::init(argc, argv);
  rclcpp::NodeOptions options;

  static rclcpp::Node::SharedPtr node = rclcpp::Node::make_shared("image_publisher", options);

  capture.set(cv::CAP_PROP_EXPOSURE, 0);
  capture.set(cv::CAP_PROP_FRAME_WIDTH, 1024);
  capture.set(cv::CAP_PROP_FRAME_HEIGHT, 2048);
  capture.set(cv::CAP_PROP_FPS, 30);

  std::thread cameraThread(camera_thread, std::ref(capture), std::ref(manager), std::ref(node));

  // Handel term signals
  signal(SIGTERM, signal_handler);
  signal(2, signal_handler);

  svr.Get("/stream", [](const httplib::Request &, httplib::Response &res)
          { res.set_chunked_content_provider(
                "multipart/x-mixed-replace; boundary=frame", // Content type
                [&](size_t offset, httplib::DataSink &sink)
                {
                  std::vector<uchar> video_data = (manager.get_buffer());

                  std::string preheader(
                      std::format("--frame\r\nContent-Type: "
                                  "image/jpeg\r\nContent-Length: {}\r\n\r\n",
                                  video_data.size()));
                  std::string payload(video_data.begin(), video_data.end());
                  std::string end("\r\n");

                  sink.write(preheader.data(), preheader.size());
                  sink.write(payload.data(), payload.size());
                  sink.write(end.data(), end.size());
                  sink.os.flush();

                  return true; // return 'false' if you want to cancel the process.
                }); });

  cameraThread.detach();

  std::cout << "Server running!" << std::endl;
  svr.listen("0.0.0.0", 8080);
}