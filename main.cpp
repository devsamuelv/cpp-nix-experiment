#include <iostream>
#include <vector>
#include <opencv4/opencv2/opencv.hpp>
#include <httplib.h>

int main(int, char **)
{
  cv::VideoCapture capture("/dev/video0");
  cv::Mat frame;

  // HTTP
  httplib::Server svr;

  svr.Get("/hi", [](const httplib::Request &, httplib::Response &res)
          { res.set_content("Hello World!", "text/plain"); });

  svr.Get("/camera", [](const httplib::Request &, httplib::Response &res)
          { for (int i = 0; i < 5; i++) {
            res.body.push_back(0xFF);
          } });

  svr.listen("0.0.0.0", 8080);
}
