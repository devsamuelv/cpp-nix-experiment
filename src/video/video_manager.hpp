#pragma once

#include <opencv4/opencv2/opencv.hpp>
#include <mutex>
#include <vector>
#include <iostream>

class VideoManager
{
private:
  std::mutex buffer_mutex;
  std::shared_ptr<std::vector<uchar>> buffer =
      std::make_shared<std::vector<uchar>>();

public:
  void update_buffer(cv::Mat mat)
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