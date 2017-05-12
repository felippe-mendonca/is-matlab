#include "mex.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <is/is.hpp>
#include <is/msgs.hpp>
#include <map>
#include <string>

static std::thread thread;
static std::atomic_bool running(false);
using namespace std::chrono_literals;

//  mex app.cpp -lSimpleAmqpClient -v CXXFLAGS='$CXXFLAGS -std=c++14'

void run(int n_out, mxArray *outputs[], int n_in, const mxArray *inputs[]) {
  if (!running) {
    running = true;
    thread = std::thread([&] {
      auto is = is::connect("amqp://guest:guest@localhost:5672");
      auto tag = is.subscribe("hodor");

      while (running) {
        auto message = is.consume_for(tag, 1s);
        if (message != nullptr) {
        }
        // is::log::info("{}", message->Message()->Body());
      }
    });
  }
}

void stop(int n_out, mxArray *outputs[], int n_in, const mxArray *inputs[]) {
  if (running) {
    running = false;
    thread.join();
  }
}

void mexFunction(int n_out, mxArray *outputs[], int n_in,
                 const mxArray *inputs[]) {

  mxArray *input;
  input = mxCreateDoubleMatrix(10, 10, mxCOMPLEX);
  double *matrix = mxGetPr(input);
  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 10; ++j) {
      matrix[j + 10 * i] = 1.0;
    }
  }
  mexCallMATLAB(0, NULL, 1, &input, "disp");
  mxDestroyArray(input);

  if (n_in == 0) {
    mexErrMsgIdAndTxt("is", "Expected atleast one argument.");
  }

  using handle_t = std::function<void(int, mxArray **, int, const mxArray **)>;
  std::map<std::string, handle_t> handlers{{"run", run}, {"stop", stop}};

  char *key = mxArrayToString(inputs[0]);
  auto &&iterator = handlers.find(key);
  if (iterator != handlers.end()) {
    iterator->second(n_out, outputs, n_in - 1, inputs + 1);
  }
}