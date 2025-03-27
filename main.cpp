#include <iostream>
#include <vector>
#include <opencv4/opencv2/opencv.hpp>

int main(int, char **)
{
  cv::VideoCapture capture("/dev/video0");
  cv::Mat frame;

  capture.read(frame);

  cv::imshow("Hello", frame);
  cv::waitKey(3);
}
