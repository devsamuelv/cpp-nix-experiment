#include "image_transport/image_transport.hpp"
#include "torch_helper.hpp"
#include "video_manager.hpp"
#include <ATen/ATen.h>
#include <csignal>
#include <cstdlib>
#include <cv_bridge/cv_bridge.h>
#include <functional>
#include <opencv2/core.hpp>
#include <opencv4/opencv2/opencv.hpp>
#include <optional>
#include <rclcpp/rclcpp/rclcpp.hpp>
#include <spdlog/spdlog.h>
#include <torch/library.h>
#include <torch/script.h>
#include <torch/torch.h>

class InferenceThread {
private:
  // Heap declared array
  inline static cv::Vec3b *colormap = new cv::Vec3b[19]{
      cv::Vec3b(128, 64, 128),  cv::Vec3b(244, 35, 232),
      cv::Vec3b(70, 70, 70),    cv::Vec3b(102, 102, 156),
      cv::Vec3b(190, 153, 153), cv::Vec3b(153, 153, 153),
      cv::Vec3b(250, 170, 30),  cv::Vec3b(220, 220, 0),
      cv::Vec3b(107, 142, 35),  cv::Vec3b(152, 251, 152),
      cv::Vec3b(0, 130, 180),   cv::Vec3b(220, 20, 60),
      cv::Vec3b(255, 0, 0),     cv::Vec3b(0, 0, 142),
      cv::Vec3b(0, 0, 70),      cv::Vec3b(0, 60, 100),
      cv::Vec3b(0, 80, 100),    cv::Vec3b(0, 0, 230),
      cv::Vec3b(119, 11, 32),
  };

  static cv::Mat applyFilter(cv::Mat *image) {
    cv::Size size((*image).cols, (*image).rows);

    // Mat is allocated on stack and is using RAII to deallocate
    // Heap allocated is terrible for this usecase since its inside a loop and
    // we don't want a mem eater
    cv::Mat colorMat(size, CV_8UC3);

    for (int x = 0; x < (*image).rows; x++) {
      cv::Mat mat = image->row(x);

      for (int y = 0; y < mat.cols; y++) {
        uchar c = (*image).at<uchar>(x, y);
        cv::Vec3b point = colormap[c];

        auto color_mat_pointer = (&colorMat)->ptr<cv::Vec3b>(x, y);

        *color_mat_pointer = point;
      }
    }

    return colorMat;
  }

public:
  static void
  camera_thread(std::reference_wrapper<cv::VideoCapture> cap,
                std::reference_wrapper<VideoManager> manager,
                std::reference_wrapper<rclcpp::Node::SharedPtr> node) {
    // I think the model needs to be transfered to torchscript. I don't want to
    // write a c++ version.
    torch::jit::Module model;

    const std::string path = "/home/samuel/code/cpp-nix-experiment/"
                             "model.pt";
    model = torch::jit::load(path);

    // Enable pytorch inference
    torch::InferenceMode(true);
    at::Device device = at::kCPU;
    cv::Mat frame;

    // RCL
    image_transport::ImageTransport it{node.get()};
    static image_transport::Publisher pub = it.advertise("camera/image", 1);
    static image_transport::Publisher pub_raw =
        it.advertise("camera/image_raw", 1);

    auto now = std::chrono::system_clock::now();
    auto now_raw = std::chrono::system_clock::now();

    model.eval();
    model.to(device);

    if (cap.get().isOpened() == false) {
      spdlog::error("Camera is not opened!");
    }

    while (cap.get().isOpened()) {
      cap.get().read(frame);

      cv::MatSize inital_frame_size = frame.size;

      auto duration_raw = now_raw.time_since_epoch();
      auto duration_ms_raw =
          std::chrono::duration_cast<std::chrono::milliseconds>(duration_raw)
              .count();

      if (rclcpp::ok() && duration_ms_raw > 150) {
        std_msgs::msg::Header hdr;
        sensor_msgs::msg::Image::SharedPtr msg;

        msg = cv_bridge::CvImage(hdr, "bgr8", frame).toImageMsg();
        pub_raw.publish(msg);
        rclcpp::spin_some(node);
        now_raw = std::chrono::system_clock::now();
      }

      // If no video data exit while and finish thread.
      if (inital_frame_size.dims() <= 0) {
        spdlog::error("No video data available");
        return;
      }

      // Convert the color scheme and channels to a torch transferable format
      // (CV_32FC3) 32-bit data with 3 channels.
      frame.convertTo(frame, CV_32FC3, 1.0 / 255.0);
      at::Tensor image_tensor =
          torch::from_blob(frame.data,
                           {frame.rows, frame.cols, frame.channels()},
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
      auto duration_ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(duration)
              .count();

      if (rclcpp::ok() && duration_ms > 150) {
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
};