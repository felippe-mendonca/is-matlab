#include <boost/program_options.hpp>
#include <is/is.hpp>
#include <is/msgs/camera.hpp>
#include <is/msgs/common.hpp>
#include <string>
#include <vector>
#include <chrono>
#include "yaml-configure.hpp"

namespace po = boost::program_options;
using namespace is::msg::camera;
using namespace is::msg::common;

int main(int argc, char* argv[]) {
  std::string uri;
  std::string yaml_file;
 
  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<std::string>(&uri)->default_value("amqp://192.168.1.110:30000"), "broker uri");
  options("yaml-file,y", po::value<std::string>(&yaml_file)->default_value("configuration.yaml"), "configuration file");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << description << std::endl;
    return 1;
  }

  auto configurations = is::camera::configuration::from_file(yaml_file);

  auto is = is::connect(uri);
  auto client = is::make_client(is);

  std::vector<std::string> ids;
  for (auto& config : configurations) {
    is::log::info("Configuring {}", config.first);
    ids.push_back(client.request(config.first + ".set_configuration", is::msgpack(config.second)));
  }

  client.receive_until(std::chrono::high_resolution_clock::now() + 1s, ids, is::policy::discard_others);
  return 0;
}