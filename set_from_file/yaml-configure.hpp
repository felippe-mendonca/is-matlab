#ifndef __YAML_CONFIGURE_HPP__
#define __YAML_CONFIGURE_HPP__

#include <yaml-cpp/yaml.h>
#include <is/msgs/camera.hpp>
#include <is/msgs/common.hpp>
#include <map>
#include <string>
#include <vector>
#include <fstream>

namespace is {
namespace camera {
namespace configuration {

using namespace is::msg::common;
using namespace is::msg::camera;
using namespace YAML;

void from_configurations(std::map<std::string, Configuration> configurations, std::string const& filename) {
  Emitter out;
  out << BeginSeq;
  for (auto& current : configurations) {
    auto camera = current.first;
    auto config = current.second;
    out << BeginMap;
    out << Key << "name" << Value << camera;
    auto sampling_rate = *(config.sampling_rate);
    out << Key << "fps" << Value << *(sampling_rate.rate);
    auto image_type = *(config.image_type);
    out << Key << "type" << Value << image_type.value;
    out << Key << "resolution";
      out << BeginMap;
      auto resolution = *(config.resolution);
      out << Key << "width" << Value << resolution.width;
      out << Key << "height" << Value << resolution.height;
      out << EndMap;
    auto brightness = *(config.brightness);
    out << Key << "brightness" << Value << brightness << Comment(" [1.367~7.422]");
    out << Key << "exposure";
      out << BeginMap;
      auto exposure = *(config.exposure);
      out << Key << "mode" << Value << *(exposure.auto_mode);
      out << Key << "value" << Value << exposure.value << Comment(" [-7.585~2.414] (0.858)");
      out << EndMap;
    out << Key << "shutter";
      out << BeginMap;
      auto shutter = *(config.shutter);
      out << Key << "mode" << Value << *(shutter.auto_mode);
      out << Key << "percent" << Value << *(shutter.percent);
      out << EndMap;
    out << Key << "gain";
      out << BeginMap;
      auto gain = *(config.gain);
      out << Key << "mode" << Value << *(gain.auto_mode);
      out << Key << "percent" << Value << *(gain.percent);
      out << EndMap;
    out << Key << "white_balance";
      out << BeginMap;
      auto white_balance = *(config.white_balance);
      out << Key << "mode" << Value << *(white_balance.auto_mode);
      out << Key << "red" << Value << *(white_balance.red) << Comment(" 0~1023");
      out << Key << "blue" << Value << *(white_balance.blue) << Comment(" 0~1023");
      out << EndMap;
    out << EndMap;
  }

  std::ofstream file;
  file.open(filename);
  file << out.c_str();
  file.close();
}

std::map<std::string, Configuration> from_file(std::string const& filename) {
  Node yaml = LoadFile(filename);
  std::map<std::string, Configuration> configurations;
  for (auto&& camera : yaml) {
    Configuration config;
    if (camera["fps"]) {
      SamplingRate sampling_rate;
      // alow alow eu n sou um vetor... sou um mapa .. vou retornar first/second
      sampling_rate.rate = camera["fps"].as<double>();
      config.sampling_rate = sampling_rate;
    }
    if (camera["type"]) {
      ImageType image_type;
      image_type.value = camera["type"].as<std::string>();
      config.image_type = image_type;
    }
    if (camera["resolution"] && camera["resolution"]["width"] && camera["resolution"]["height"]) {
      Resolution resolution;
      resolution.width = camera["resolution"]["width"].as<unsigned int>();
      resolution.height = camera["resolution"]["height"].as<unsigned int>();
      config.resolution = resolution;
    }
    if (camera["exposure"] && camera["exposure"]["mode"] && camera["exposure"]["value"]) {
      Exposure exposure;
      exposure.auto_mode = !(camera["exposure"]["mode"].as<std::string>() == "manual");
      exposure.value = camera["exposure"]["value"].as<double>();
      config.exposure = exposure;
    }
    if (camera["brightness"]) {
      config.brightness = camera["brightness"].as<float>();
    }
    if (camera["shutter"] && camera["shutter"]["mode"] && (camera["shutter"]["percent"] || camera["shutter"]["ms"])) {
      Shutter shutter;
      shutter.auto_mode = !(camera["shutter"]["mode"].as<std::string>() == "manual");
      if (camera["shutter"]["percent"]) {
        shutter.percent = camera["shutter"]["percent"].as<float>();
      }
      if (camera["shutter"]["ms"]) {
        shutter.ms = camera["shutter"]["ms"].as<float>();
      }
      config.shutter = shutter;
    }
    if (camera["gain"] && camera["gain"]["mode"] && camera["gain"]["percent"]) {
      Gain gain;
      gain.auto_mode = !(camera["gain"]["mode"].as<std::string>() == "manual");
      gain.percent = camera["gain"]["percent"].as<double>();
      config.gain = gain;
    }
    if (camera["white_balance"] && camera["white_balance"]["mode"] && camera["white_balance"]["red"] &&
        camera["white_balance"]["blue"]) {
            WhiteBalance white_balance;
      white_balance.auto_mode = !(camera["white_balance"]["mode"].as<std::string>() == "manual");
      white_balance.red = camera["white_balance"]["red"].as<unsigned int>();
      white_balance.blue = camera["white_balance"]["blue"].as<unsigned int>();
      config.white_balance = white_balance;
    }
    configurations.emplace(camera["name"].as<std::string>(), config);
  }
  return configurations;
}

}  // ::configuration
}  // ::camera
}  // ::is

#endif  // __YAML_CONFIGURE_HPP__