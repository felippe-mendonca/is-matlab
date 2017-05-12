#include "mex.h"

#include <functional>
#include <iostream>
#include <is/is.hpp>
#include <is/msgs/camera.hpp>
#include <map>
#include <opencv2/highgui.hpp>
#include <string>

// mex is.cpp -lSimpleAmqpClient -lopencv_imgcodecs -lopencv_core -v CXXFLAGS='$CXXFLAGS -std=c++14'

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

  auto message = connection->consume(kv->second);
  auto image = is::msgpack<is::msg::camera::CompressedImage>(message);
  cv::Mat frame = cv::imdecode(image.data, CV_LOAD_IMAGE_COLOR);
  mwSize dims[3] = {frame.rows, frame.cols, 1};

  // O PIRU TA AQUI!!!
  outputs[0] = mxCreateNumericArray(3, dims, mxUINT8_CLASS, mxREAL);
  unsigned char *matrix = (unsigned char *)mxGetData(outputs[0]);

  std::vector<cv::Mat> channels;
  cv::split(frame, channels);
  std::copy(channels[0].datastart, channels[0].dataend, matrix);
}

// AQUI PRA TESTAR O PIRU
void test(int n_out, mxArray *outputs[], int n_in, const mxArray *inputs[]) {
  if (n_in != 1) {
    mexErrMsgIdAndTxt("is:InvalidArgCount", "Expected atleast one argument.");
  }

  cv::Mat[] = {cv::ones(10, 5, CV_U8), cv::ones(10, 5, CV_U8), cv::ones(10, 5, CV_U8)} 

  const char *path = mxArrayToString(inputs[0]);

  cv::Mat frame = cv::imread(path, CV_LOAD_IMAGE_COLOR);
  mwSize dims[3] = {frame.rows, frame.cols, 1};

  outputs[0] = mxCreateNumericArray(3, dims, mxUINT8_CLASS, mxREAL);
  unsigned char *matrix = (unsigned char *)mxGetData(outputs[0]);

  std::vector<cv::Mat> channels;
  cv::split(frame, channels);
  std::copy(channels[0].datastart, channels[0].dataend, matrix);
}

} // ::mex

using handle_t = std::function<void(int, mxArray **, int, const mxArray **)>;
std::map<std::string, handle_t> handlers{{"connect", mex::connect},
                                         {"close", mex::close},
                                         {"subscribe", mex::subscribe},
                                         {"consume_frame", mex::consume_frame},
                                         {"test", mex::test}};

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