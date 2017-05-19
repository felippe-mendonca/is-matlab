#include "MxArray.hpp"
#include "mex.h"
#include <functional>
#include <iostream>
#include <is/is.hpp>
#include <is/msgs/camera.hpp>
#include <map>
#include <opencv2/highgui.hpp>
#include <string>

namespace mex {

std::unique_ptr<is::Connection> connection;
std::map<std::string, is::QueueInfo> subscriptions;

void clean() {
  subscriptions.clear();
  connection = nullptr;
}

void close(int n_out, mxArray *outputs[], int n_in, const mxArray *inputs[]) {
  clean();
}

void connect(int n_out, mxArray *outputs[], int n_in, const mxArray *inputs[]) {
  const char *uri =
      (n_in == 0) ? "amqp://localhost" : mxArrayToString(inputs[0]);

  connection = std::make_unique<is::Connection>(uri);
  mexAtExit(clean);
}

void subscribe(int n_out, mxArray *outputs[], int n_in,
               const mxArray *inputs[]) {
  if (n_in != 1) {
    mexErrMsgIdAndTxt("is:InvalidArgCount", "Expected atleast one argument.");
  }
  if (connection == nullptr) {
    mexErrMsgIdAndTxt("is:NotConnected", "Not connected");
  }

  const char *topic = mxArrayToString(inputs[0]);

  auto kv = subscriptions.find(topic);
  if (kv == subscriptions.end()) {
    auto info = connection->subscribe(topic);
    subscriptions.emplace(topic, info);
    is::log::info("Subscribe to topic {} at {}", topic, info.name);
  }
}

void consume_frame(int n_out, mxArray *outputs[], int n_in,
                   const mxArray *inputs[]) {
  if (n_in != 1) {
    mexErrMsgIdAndTxt("is:InvalidArgCount", "Expected atleast one argument.");
  }

  if (connection == nullptr) {
    mexErrMsgIdAndTxt("is:NotConnected", "Not connected");
  }

  const char *topic = mxArrayToString(inputs[0]);

  auto kv = subscriptions.find(topic);
  if (kv == subscriptions.end()) {
    mexErrMsgIdAndTxt("is:NoSuchTopic", "Not subscribed to this topic");
  }

  auto before = std::chrono::high_resolution_clock::now();
  auto message = connection->consume(kv->second);
  auto after = std::chrono::high_resolution_clock::now();
  is::log::info(">{}", std::chrono::duration_cast<std::chrono::milliseconds>(
                           after - before)
                           .count());

  auto image = is::msgpack<is::msg::camera::CompressedImage>(message);
  cv::Mat frame = cv::imdecode(image.data, CV_LOAD_IMAGE_COLOR);
  if (frame.channels() == 3) {
    cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
  } else if (frame.channels() == 4) {
    cv::cvtColor(frame, frame, cv::COLOR_BGRA2RGBA);
  }

  outputs[0] = MxArray(frame);
}

} // ::mex

using handle_t = std::function<void(int, mxArray **, int, const mxArray **)>;
std::map<std::string, handle_t> handlers{{"connect", mex::connect},
                                         {"close", mex::close},
                                         {"subscribe", mex::subscribe},
                                         {"consume_frame", mex::consume_frame}};

void mexFunction(int n_out, mxArray *outputs[], int n_in,
                 const mxArray *inputs[]) {
  if (n_in == 0) {
    mexErrMsgIdAndTxt("is:InvalidArgCount", "Expected atleast one argument.");
  }
  char *key = mxArrayToString(inputs[0]);
  auto &&iterator = handlers.find(key);
  if (iterator != handlers.end()) {
    iterator->second(n_out, outputs, n_in - 1, inputs + 1);
  } else {
    mexErrMsgIdAndTxt("is:InvalidArg", "Invalid argument");
  }
}