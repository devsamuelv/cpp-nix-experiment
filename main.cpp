#include <iostream>
#include <vector>
#include <opencv4/opencv2/opencv.hpp>
#include <string>
#include <httplib.h>
#include <algorithm>
#include <format>

int main(int, char **)
{

  // HTTP
  httplib::Server svr;

  svr.Get("/hi", [](const httplib::Request &, httplib::Response &res)
          { res.set_content("Hello World!", "text/plain"); });

  svr.Get("/camera", [](const httplib::Request &, httplib::Response &res)
          {
            cv::Mat frame;
            std::vector<uchar> buff;

            // while (capture.isOpened())
            // {
            //   bool hasFrame = capture.read(frame);
            //   cv::imencode(".jpg", frame, buff);
            //   std::string payload(buff.begin(), buff.end());

            // }
            // prepare data...

            res.set_header("content-type", "multipart/x-mixed-replace");

            res.set_content_provider(
                "image/jpeg", // Content type
                [&](size_t offset, httplib::DataSink &sink)
                {
                  cv::VideoCapture capture("/dev/video0", cv::CAP_V4L2);
                  cv::Mat frame;
                  std::vector<uchar> buff;
                  bool hasFrame = capture.read(frame);

                  if (hasFrame)
                  {
                    cv::imencode(".jpg", frame, buff);
                    // std::string starter(std::format("--frame\r\n\r\nContent-Length: {}\r\n\r\n", buff.size()));
                    std::string payload(buff.begin(), buff.end());
                    std::string end("\n\b");
                    // prepare data...
                    // sink.write(starter.data(), starter.size());
                    sink.write(payload.data(), payload.size());
                    sink.write(end.data(), end.size());
                    // sink.os.flush();
                  }
                  return true; // return 'false' if you want to cancel the process.
                }); });

  svr.listen("0.0.0.0", 8080);
}
