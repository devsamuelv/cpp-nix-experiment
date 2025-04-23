#include <iostream>
#include <vector>
#include <opencv4/opencv2/opencv.hpp>
#include <string>
#include <httplib.h>
#include <algorithm>
#include <format>

void camera_thread(std::reference_wrapper<cv::VideoCapture> cap, std::reference_wrapper<std::shared_ptr<std::vector<uchar>>> buff)
{
  while (true)
  {
    if (cap.get().isOpened())
    {
      cv::Mat frame;
      bool containsImage = cap.get().read(frame);

      if (containsImage)
      {
        cv::imencode(".jpg", frame, *buff.get());
        std::cout << "Updating Frame" << std::endl;
      }
    }
  }
}

size_t getSize(std::shared_ptr<std::vector<uchar>> a)
{
  return (*a).size();
}

int main(int, char **)
{
  static cv::VideoCapture capture("/dev/video0", cv::CAP_V4L2);
  static std::shared_ptr<std::vector<uchar>> cam_buff = std::make_shared<std::vector<uchar>>();

  capture.set(cv::CAP_PROP_EXPOSURE, 0);
  capture.set(cv::CAP_PROP_FRAME_WIDTH, 720);
  capture.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);

  std::jthread cameraThread(
      camera_thread,
      std::ref(capture),
      std::ref(cam_buff));

  // HTTP
  httplib::Server svr;

  svr.Get("/hi", [](const httplib::Request &, httplib::Response &res)
          { res.set_chunked_content_provider(
                "multipart/x-mixed-replace; boundary=frame", // Content type
                [&](size_t offset, httplib::DataSink &sink)
                {
                  std::vector<uchar> buff = *cam_buff;
                  std::string preheader(std::format("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: {}\r\n\r\n", buff.size()));
                  std::string payload(buff.begin(), buff.end());
                  std::string end("\r\n");

                  sink.write(preheader.data(), preheader.size());
                  sink.write(payload.data(), payload.size());
                  sink.write(end.data(), end.size());
                  sink.os.flush();
                  return true; // return 'false' if you want to cancel the process.
                }); });

  // svr.Get("/camera", [](const httplib::Request &, httplib::Response &res)
  //         {
  //           cv::Mat frame;
  //           std::vector<uchar> buff;

  //           res.set_chunked_content_provider(
  //               "multipart/x-mixed-replace; boundary=frame", // Content type
  //               [&](size_t offset, httplib::DataSink &sink)
  //               {
  //                 cv::Mat frame;
  //                 std::vector<uchar> buff;
  //                 bool hasFrame = capture.read(frame);

  //                 if (hasFrame)
  //                 {
  //                   cv::imencode(".jpg", frame, buff);
  //                   std::string preheader(std::format("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: {}\r\n\r\n", buff.size()));
  //                   std::string payload(buff.begin(), buff.end());
  //                   std::string end("\r\n");

  //                   sink.write(preheader.data(), preheader.size());
  //                   sink.write(payload.data(), payload.size());
  //                   sink.write(end.data(), end.size());
  //                   sink.os.flush();
  //                 }
  //                 return true; // return 'false' if you want to cancel the process.
  //               }); });

  std::cout << "Server running!" << std::endl;

  svr.listen("0.0.0.0", 8080);
  cameraThread.join();
}