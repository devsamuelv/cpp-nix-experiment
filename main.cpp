#include <iostream>
#include <vector>
#include <opencv4/opencv2/opencv.hpp>
#include <string>
#include <httplib.h>
#include <algorithm>
#include <format>

int main(int, char **)
{
  static cv::VideoCapture capture("/dev/video0", cv::CAP_V4L2);

  // HTTP
  httplib::Server svr;

  svr.Get("/hi", [](const httplib::Request &, httplib::Response &res)
          { res.set_content("Hello World!", "text/plain"); });

  svr.Get("/camera", [](const httplib::Request &, httplib::Response &res)
          {
            cv::Mat frame;
            std::vector<uchar> buff;

            res.set_header("content-type", "multipart/x-mixed-replace");

            res.set_chunked_content_provider(
                "image/jpeg", // Content type
                [&](size_t offset, httplib::DataSink &sink)
                {
                  cv::Mat frame;
                  std::vector<uchar> buff;
                  bool hasFrame = capture.read(frame);

                  if (hasFrame)
                  {
                    cv::imencode(".jpg", frame, buff);
                    std::string payload(buff.begin(), buff.end());
                    sink.write(payload.data(), payload.size());
                    sink.os.flush();
                  }
                  return true; // return 'false' if you want to cancel the process.
                }); });

  svr.listen("0.0.0.0", 8080);
}
