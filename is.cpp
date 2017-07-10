#include "MxArray.hpp"
#include "matrix.h"
#include "mex.h"
#include <armadillo>
#include <functional>
#include <iostream>
#include <is/is.hpp>
#include <is/msgs/camera.hpp>
#include <is/msgs/common.hpp>
#include <map>
#include <opencv2/highgui.hpp>
#include <string>
#include <string>
#include "arma.hpp"

namespace mex {

std::unique_ptr<is::Connection> connection;
std::unique_ptr<is::ServiceClient> client;
std::map<std::string, is::QueueInfo> subscriptions;

using namespace arma;

//  creates an armadillo matrix from a matlab matrix
mat armaMatrix(const mxArray *matlabMatrix[]) {
  mwSize nrows = mxGetM(matlabMatrix[0]);
  mwSize ncols = mxGetN(matlabMatrix[0]);
  double *values = mxGetPr(matlabMatrix[0]);
  return mat(values, nrows, ncols);
}

void clean() {
  subscriptions.clear();
  client = nullptr;
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
  // is::log::info(">{}", std::chrono::duration_cast<std::chrono::milliseconds>(
  //                          after - before)
  //                          .count());

  auto image = is::msgpack<is::msg::camera::CompressedImage>(message);
  cv::Mat frame = cv::imdecode(image.data, CV_LOAD_IMAGE_COLOR);
  if (frame.channels() == 3) {
    cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
  } else if (frame.channels() == 4) {
    cv::cvtColor(frame, frame, cv::COLOR_BGRA2RGBA);
  }

  outputs[0] = MxArray(frame);
}

void set_sample_rate(int n_out, mxArray *outputs[], int n_in,
                     const mxArray *inputs[]) {

  if (n_in < 2) {
    mexErrMsgIdAndTxt("is:InvalidArgCount", "Expected at least tow arguments.");
  }

  if (!mxIsScalar(inputs[0])) {
    mexErrMsgIdAndTxt("is:InvalidArgType", "First argument must be a scalar.");
  }

  if (!mxIsCell(inputs[1])) {
    mexErrMsgIdAndTxt("is:InvalidArgType",
                      "Second argument must be a cell array.");
  }

  if (connection == nullptr) {
    mexErrMsgIdAndTxt("is:NotConnected", "Not connected");
  }

  if (client == nullptr) {
    client = std::make_unique<is::ServiceClient>(connection->channel);
  }

  is::msg::common::SamplingRate sampling_rate;
  sampling_rate.rate = mxGetScalar(inputs[0]);

  const mwSize *n_elements;
  n_elements = mxGetDimensions(inputs[1]);
  mxArray *cellElement;
  for (int i = 0; i < n_elements[1]; i++) {
    cellElement = mxGetCell(inputs[1], i);
    char *camera = mxArrayToString(cellElement);
    client->request(std::string(camera) + ".set_sample_rate",
                    is::msgpack(sampling_rate));
  }
}

void publish_bbs(int n_out, mxArray *outputs[], int n_in,
                 const mxArray *inputs[]) {
  if (n_in < 2) {
    mexErrMsgIdAndTxt("is:InvalidArgCount", "Expected at least tow arguments.");
  }

  if (!mxIsChar(inputs[0])) {
    mexErrMsgIdAndTxt("is:InvalidArgType", "First argument must be a string.");
  }

  // if (!mxIsArray(inputs[1])) {
  //   mexErrMsgIdAndTxt("is:InvalidArgType", "Second argument must be a
  //   matrix.");
  // }

  if (connection == nullptr) {
    mexErrMsgIdAndTxt("is:NotConnected", "Not connected");
  }

  if (client == nullptr) {
    client = std::make_unique<is::ServiceClient>(connection->channel);
  }

  const char *topic = mxArrayToString(inputs[0]);
  mat bbs = armaMatrix(&inputs[1]); // matrix Nx5 (x, y, w, h, score)
  connection->publish(topic, is::msgpack(bbs));
}

} // ::mex

using handle_t = std::function<void(int, mxArray **, int, const mxArray **)>;
std::map<std::string, handle_t> handlers{
    {"connect", mex::connect},
    {"close", mex::close},
    {"subscribe", mex::subscribe},
    {"consume_frame", mex::consume_frame},
    {"set_sample_rate", mex::set_sample_rate},
    {"publish_bbs", mex::publish_bbs}};

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