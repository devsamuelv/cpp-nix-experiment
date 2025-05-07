#include <iostream>
#include <vector>
#include <opencv4/opencv2/opencv.hpp>
#include <string>
#include <httplib.h>
#include <algorithm>
#include <format>
#include <mutex>
#include <thread>

class VideoManager
{
private:
  std::mutex buffer_mutex;
  std::shared_ptr<std::vector<uchar>> buffer = std::make_shared<std::vector<uchar>>();

public:
  void
  update_buffer(cv::Mat mat)
  {
    std::unique_lock<std::mutex> lock(buffer_mutex);
    cv::imencode(".jpg", mat, *buffer);
    lock.unlock();
  };

  std::vector<uchar> get_buffer()
  {
    std::lock_guard lock(buffer_mutex);
    return *buffer.get();
  };
};

void camera_thread(std::reference_wrapper<cv::VideoCapture> cap, std::reference_wrapper<VideoManager> manager)
{
  while (cap.get().isOpened())
  {
    cv::Mat frame;
    cap.get().read(frame);
    manager.get().update_buffer(frame);
  }
}

size_t get_size(std::shared_ptr<std::vector<uchar>> a)
{
  return (*a).size();
}

int main(int, char **)
{
  static cv::VideoCapture capture("/dev/video0", cv::CAP_V4L2);
  static VideoManager manager;

  capture.set(cv::CAP_PROP_EXPOSURE, 0);
  capture.set(cv::CAP_PROP_FRAME_WIDTH, 720);
  capture.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);

  std::thread cameraThread(
      camera_thread,
      std::ref(capture),
      std::ref(manager));

  // HTTP
  httplib::Server svr;

  svr.Get("/stream", [](const httplib::Request &, httplib::Response &res)
          { res.set_chunked_content_provider(
                "multipart/x-mixed-replace; boundary=frame", // Content type
                [&](size_t offset, httplib::DataSink &sink)
                {
                  std::vector<uchar> video_data = (manager.get_buffer());

                  std::string preheader(std::format("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: {}\r\n\r\n", video_data.size()));
                  std::string payload(video_data.begin(), video_data.end());
                  std::string end("\r\n");

                  sink.write(preheader.data(), preheader.size());
                  sink.write(payload.data(), payload.size());
                  sink.write(end.data(), end.size());
                  sink.os.flush();

                  return true; // return 'false' if you want to cancel the process.
                }); });

  std::cout << "Server running!" << std::endl;

  svr.listen("0.0.0.0", 8080);
  cameraThread.detach();
}