#include <armadillo>
#include <boost/iterator/zip_iterator.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <is/is.hpp>
#include <is/msgs/camera.hpp>
#include <is/msgs/common.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <string>
#include <vector>
#include <iomanip>
#include "../arma.hpp"

namespace po = boost::program_options;
using namespace is::msg::camera;
using namespace is::msg::common;
using namespace std::chrono;
using namespace arma;

namespace std {
std::ostream& operator<<(std::ostream& os, const std::vector<std::string>& vec) {
  for (auto item : vec) {
    os << item << " ";
  }
  return os;
}
}

int main(int argc, char* argv[]) {
  std::string uri;
  std::vector<std::string> cameras;
  const std::vector<std::string> default_cameras{"ptgrey.0", "ptgrey.1", "ptgrey.2", "ptgrey.3"};

  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<std::string>(&uri)->default_value("amqp://192.168.1.110:30000"), "broker uri");
  options("cameras,c", po::value<std::vector<std::string>>(&cameras)->multitoken()->default_value(default_cameras),
          "cameras");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << description << std::endl;
    return 1;
  }

  auto is = is::connect(uri);
  auto client = is::make_client(is);

  std::vector<std::string> ids;
  for (auto& camera : cameras) {
    ids.push_back(client.request(camera + ".get_configuration", is::msgpack(0)));
  }
  auto configuration_msgs = client.receive_until(high_resolution_clock::now() + 1s, ids, is::policy::discard_others);
  if (configuration_msgs.size() != cameras.size())
    exit(0);

  auto configuration = is::msgpack<Configuration>(configuration_msgs.at(ids.front()));
  int64_t period = static_cast<int64_t>(1000.0 / *((*(configuration.sampling_rate)).rate));

  std::vector<std::string> topics;
  for (auto& camera : cameras) {
    topics.push_back(camera + ".frame");
  }
  auto tag = is.subscribe(topics);

  std::vector<std::string> bbs_topics;
  for (auto& camera : cameras) {
    bbs_topics.push_back(camera + ".bbs");
  }
  auto bbs_tag = is.subscribe(bbs_topics);

  is::log::info("Starting capture. Period: {} ms", period);

  while (1) {
    auto images_msg = is.consume_sync(tag, topics, period);
    auto bbs_msg = is.consume_sync(bbs_tag, bbs_topics, period);

    auto it_begin = boost::make_zip_iterator(boost::make_tuple(images_msg.begin(), bbs_msg.begin()));
    auto it_end = boost::make_zip_iterator(boost::make_tuple(images_msg.end(), bbs_msg.end()));
    std::vector<cv::Mat> up_frames, down_frames;
    int n_frame = 0;
    std::for_each(it_begin, it_end, [&](auto msgs) {
      auto image_msg = boost::get<0>(msgs);
      auto bbs_msg = boost::get<1>(msgs);
      auto image = is::msgpack<CompressedImage>(image_msg);
      arma::mat bbs = is::msgpack<arma::mat>(bbs_msg);

      cv::Mat current_frame = cv::imdecode(image.data, CV_LOAD_IMAGE_COLOR);

      bbs.each_row([&](arma::rowvec const& a) {
        cv::rectangle(current_frame, cv::Rect(a(0), a(1), a(2), a(3)), cv::Scalar(0, 255, 0), 3);
        std::stringstream ss_text;
        ss_text << std::setprecision(2) << a(4);
        std::string text = ss_text.str();
        int fontface = cv::FONT_HERSHEY_SIMPLEX;
        double scale = 1.0;
        int tickness = 2;
        int baseline = 0;
        cv::Point point(a(0) + a(2), a(1) + a(3));
        cv::Size text_size = cv::getTextSize(text, fontface, scale, tickness, &baseline);
        cv::rectangle(current_frame, point + cv::Point(0, baseline), point + cv::Point(text_size.width, -text_size.height), cv::Scalar(0,0,0), CV_FILLED);
        cv::putText(current_frame, text, point, cv::FONT_HERSHEY_SCRIPT_SIMPLEX, scale, cv::Scalar(0, 255, 255), tickness, 8);
      });

      cv::resize(current_frame, current_frame, cv::Size(current_frame.cols / 2, current_frame.rows / 2));
      if (n_frame < 2) {
        up_frames.push_back(current_frame);
      } else {
        down_frames.push_back(current_frame);
      }
      n_frame++;
    });

    cv::Mat output_image;
    cv::Mat up_row, down_row;
    std::vector<cv::Mat> rows_frames;
    cv::hconcat(up_frames, up_row);
    rows_frames.push_back(up_row);
    cv::hconcat(down_frames, down_row);
    rows_frames.push_back(down_row);
    cv::vconcat(rows_frames, output_image);

    cv::imshow("Intelligent Space", output_image);
    cv::waitKey(1);
  }

  is::logger()->info("Exiting");
  return 0;
}