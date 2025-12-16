#pragma once

#include <cstdint>
#include <opencv2/core.hpp>
#include <opencv4/opencv2/opencv.hpp>
#include <torch/torch.h>

class TorchHelpers {
public:
  static cv::Mat torchTensortoCVMat(torch::Tensor &tensor) {
    tensor = tensor.squeeze().detach();
    tensor = tensor.to(at::kByte).to(torch::kCPU);

    return cv::Mat(cv::Size(tensor.size(1), tensor.size(0)), CV_8UC1,
                   tensor.mutable_data_ptr<uchar>());
  };
};
