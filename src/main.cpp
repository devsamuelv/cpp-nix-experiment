#include "video/inference_thread.hpp"
#include "video/video_manager.hpp"
#include <ATen/ATen.h>
#include <algorithm>
#include <chrono>
#include <csignal>
#include <cv_bridge/cv_bridge.h>
#include <format>
#include <functional>
#include <httplib.h>
#include <iostream>
#include <mutex>
#include <opencv4/opencv2/opencv.hpp>
#include <rclcpp/rclcpp.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <torch/library.h>
#include <torch/script.h>
#include <torch/torch.h>
#include <torch/types.h>
#include <vector>

using namespace std::chrono_literals;

static volatile sig_atomic_t sig_caught = 0;
static httplib::Server svr;

size_t get_size(std::shared_ptr<std::vector<uchar>> a) { return (*a).size(); }

void signal_handler(int signal) {
  sig_caught = 1;

  svr.stop();

  spdlog::info("Program Terminated!");
}

int main(int argc, char *argv[]) {
  static cv::VideoCapture capture("/dev/video0", cv::CAP_ANY);
  static VideoManager manager;

  rclcpp::init(argc, argv);
  rclcpp::NodeOptions options;

  static rclcpp::Node::SharedPtr node =
      rclcpp::Node::make_shared("image_publisher", options);

  capture.set(cv::CAP_PROP_EXPOSURE, 0);
  capture.set(cv::CAP_PROP_FRAME_WIDTH, 1024);
  capture.set(cv::CAP_PROP_FRAME_HEIGHT, 2048);
  capture.set(cv::CAP_PROP_FPS, 30);

  std::thread cameraThread(&InferenceThread::camera_thread, std::ref(capture),
                           std::ref(manager), std::ref(node));

  // Handel term signals
  signal(SIGTERM, signal_handler);
  signal(2, signal_handler);

  svr.Get("/stream", [](const httplib::Request &, httplib::Response &res) {
    res.set_chunked_content_provider(
        "multipart/x-mixed-replace; boundary=frame", // Content type
        [&](size_t offset, httplib::DataSink &sink) {
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
        });
  });

  cameraThread.detach();

  spdlog::info("Webserver running!");
  svr.listen("0.0.0.0", 8080);
}