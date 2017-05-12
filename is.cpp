#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <functional>
#include <iostream>
#include <map>
#include <string>

#include "mex.h"

using namespace AmqpClient;

static Channel::ptr_t channel;

void clean() { channel = nullptr; }

void connect(int n_out, mxArray *outputs[], int n_in, const mxArray *inputs[]) {
  const char *uri =
      (n_in == 0) ? "amqp://localhost" : mxArrayToString(inputs[0]);

  std::cout << "Connecting to " << uri << std::endl;
  channel = Channel::CreateFromUri(uri);
  mexAtExit(clean);
}

void subscribe(int n_out, mxArray *outputs[], int n_in,
               const mxArray *inputs[]) {
  if (channel == nullptr) {
    mexErrMsgIdAndTxt("is:subscribe", "No connection to broker.");
  }

  if (n_in == 0) {
    mexErrMsgIdAndTxt("is:subscribe", "No topic was specified.");
  }

  bool passive{false};
  bool durable{false};
  bool exclusive{true};
  bool auto_delete{true};
  Table arguments{{TableKey("x-max-length"), TableValue(1)}};

  auto queue = channel->DeclareQueue("", passive, durable, exclusive,
                                     auto_delete, arguments);
  for (int i = 0; i < n_in; ++i) {
    auto &&topic = mxArrayToString(inputs[i]);
    channel->BindQueue(queue, "amq.topic", topic);
  }

  bool no_local{true};
  bool no_ack{false};
  auto &&tag = channel->BasicConsume(queue, "", no_local, no_ack, exclusive);

  outputs[0] = mxCreateString(tag.data());
}

void consume(int n_out, mxArray *outputs[], int n_in, const mxArray *inputs[]) {
  if (channel == nullptr) {
    mexErrMsgIdAndTxt("is:subscribe", "No connection to broker.");
  }

  if (n_in == 0) {
    mexErrMsgIdAndTxt("is:consume", "No tag was specified.");
  }
  const char *tag = mxArrayToString(inputs[0]);

  Envelope::ptr_t envelope = channel->BasicConsumeMessage(tag);
  channel->BasicAck(envelope);

  outputs[0] = mxCreateString(envelope->Message()->Body().data());
  outputs[1] = mxCreateString(envelope->RoutingKey().data());
}

void unpack_compressed_image(int n_out, mxArray *outputs[], int n_in,
                             const mxArray *inputs[]) {
                               
                             }

void mexFunction(int n_out, mxArray *outputs[], int n_in,
                 const mxArray *inputs[]) {

  if (n_in == 0) {
    mexErrMsgIdAndTxt("is", "Expected atleast one argument.");
  }

  using handle_t = std::function<void(int, mxArray **, int, const mxArray **)>;
  std::map<std::string, handle_t> handlers{
      {"connect", connect}, {"subscribe", subscribe}, {"consume", consume}};

  char *key = mxArrayToString(inputs[0]);
  auto &&iterator = handlers.find(key);
  if (iterator != handlers.end()) {
    iterator->second(n_out, outputs, n_in - 1, inputs + 1);
  }
}