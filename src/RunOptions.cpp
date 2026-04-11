#include "RunOptions.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace {

/**
 * @brief 读取一项字符串输入；若用户留空则返回默认值。
 *
 * @param prompt 提示文案。
 * @param default_prompt 展示给用户的默认值文案。
 * @param default_value 用户留空时实际采用的默认值。
 * @return string 用户输入值或默认值。
 */
string prompt_value(const string& prompt, const string& default_prompt,
                    const string& default_value) {
  cout << prompt << " (默认: " << default_prompt << "): ";
  string input;
  getline(cin, input);
  return input.empty() ? default_value : input;
}

/**
 * @brief 读取一项整数输入；若输入非法则重复提示。
 *
 * @param prompt 提示文案。
 * @param default_prompt 展示给用户的默认值文案。
 * @param default_value 用户留空时实际采用的默认值。
 * @return int 用户输入的有效整数或默认值。
 */
int prompt_int_value(const string& prompt, const string& default_prompt, int default_value) {
  while (true) {
    const string value = prompt_value(prompt, default_prompt, to_string(default_value));

    try {
      size_t parsed_length = 0;
      const int parsed_value = stoi(value, &parsed_length);
      if (parsed_length != value.size()) {
        throw invalid_argument("contains trailing characters");
      }
      return parsed_value;
    } catch (const exception&) {
      cerr << "请输入有效整数。" << endl;
    }
  }
}

/**
 * @brief 生成终点时间的交互式默认显示文案。
 *
 * @param end_minutes 终点分钟值。
 * @return string 当值为 `-1` 时返回“结尾”，否则返回对应数字字符串。
 */
string end_minutes_prompt(int end_minutes) {
  return end_minutes == -1 ? "结尾" : to_string(end_minutes);
}

} // namespace

string get_default_output_folder_name() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
  std::tm now_tm = *std::localtime(&now_c);

  std::ostringstream oss;
  oss << "output_" << std::setfill('0') << std::setw(2) << now_tm.tm_mon + 1 << std::setw(2)
      << now_tm.tm_mday << "_" << std::setw(2) << now_tm.tm_hour << std::setw(2) << now_tm.tm_min
      << std::setw(2) << now_tm.tm_sec;

  return oss.str();
}

RunOptions make_default_run_options() {
  return RunOptions{
      "1.mp4", get_default_output_folder_name(), 0, -1, 30, 5, 4, false,
  };
}

RunOptions merge_with_defaults(const PartialRunOptions& partial) {
  RunOptions options = make_default_run_options();
  if (partial.input_file.has_value()) {
    options.input_file = *partial.input_file;
  }
  if (partial.output_folder.has_value()) {
    options.output_folder = *partial.output_folder;
  }
  if (partial.start_minutes.has_value()) {
    options.start_minutes = *partial.start_minutes;
  }
  if (partial.end_minutes.has_value()) {
    options.end_minutes = *partial.end_minutes;
  }
  if (partial.frame_skip.has_value()) {
    options.frame_skip = *partial.frame_skip;
  }
  if (partial.progress_interval.has_value()) {
    options.progress_interval = *partial.progress_interval;
  }
  if (partial.threshold.has_value()) {
    options.threshold = *partial.threshold;
  }
  if (partial.quiet.has_value()) {
    options.quiet = *partial.quiet;
  }
  return options;
}

RunOptions prompt_for_options(const PartialRunOptions& seed) {
  RunOptions options = merge_with_defaults(seed);

  options.input_file = prompt_value("请输入视频文件路径", options.input_file, options.input_file);
  options.output_folder =
      prompt_value("请输入输出文件夹路径", options.output_folder, options.output_folder);
  options.start_minutes =
      prompt_int_value("请输入起点(分钟)", to_string(options.start_minutes), options.start_minutes);
  options.end_minutes = prompt_int_value(
      "请输入终点(分钟)", end_minutes_prompt(options.end_minutes), options.end_minutes);
  options.frame_skip =
      prompt_int_value("请输入跳帧检测值", to_string(options.frame_skip), options.frame_skip);
  options.progress_interval =
      prompt_int_value("请输入进度提示间隔时间(分钟)", to_string(options.progress_interval),
                       options.progress_interval);
  options.threshold =
      prompt_int_value("请输入相似度比较阈值", to_string(options.threshold), options.threshold);

  return options;
}

std::string validate_options(const RunOptions& options) {
  if (options.input_file.empty()) {
    return "视频文件路径不能为空";
  }

  if (options.output_folder.empty()) {
    return "输出文件夹路径不能为空";
  }

  if (options.start_minutes < 0) {
    return "起点不能小于 0";
  }

  if (options.end_minutes != -1 && options.end_minutes <= options.start_minutes) {
    return "终点必须大于起点，或使用 -1 表示视频结尾";
  }

  if (options.frame_skip < 1) {
    return "跳帧检测值必须大于等于 1";
  }

  if (options.progress_interval < 1) {
    return "进度提示间隔必须大于等于 1";
  }

  if (options.threshold < 0) {
    return "相似度比较阈值不能小于 0";
  }

  return "";
}
