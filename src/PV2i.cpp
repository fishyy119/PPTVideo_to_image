#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include "ArgumentParser.hpp"
#include "RunOptions.hpp"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

#define HASH_WINDOW_SIZE 3 // 相似判断的窗口
// TODO: 把这个变成自定义参数?

using namespace std;
using namespace cv;
namespace fs = std::filesystem;
// TODO: 自适应跳帧(由于主要时间开销都在帧提取上，因此对此处的优化可以极大加速)
// TODO: 更好的检测算法

/**
 * @brief 将给定的秒数格式化为 HH:MM:SS 的字符串格式。
 *
 * @param seconds 总秒数
 * @return string 格式化后的时间字符串
 */
string time_format(double seconds) {
  int hours = int(seconds / 3600);
  int minutes = int((seconds - hours * 3600) / 60);
  int secs = int(seconds) % 60;
  char buffer[10];
  snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hours, minutes, secs);
  return string(buffer);
}

// 进度报告类
class ProgressReporter {
  public:
  /**
   * @brief 构造函数
   *
   * @param total_duration 视频总时长（s）
   * @param fps 帧率
   * @param progress_interval 进度报告间隔（min）
   * @param start 提取起始帧
   * @param end 提取结束帧
   * @param enabled 是否启用进度输出
   */
  ProgressReporter(double total_duration, int fps, int progress_interval, int start, int end,
                   bool enabled)
      : total_duration(total_duration), fps(fps), progress_interval(progress_interval),
        start(start), end(end), enabled(enabled) {

    start_time = chrono::high_resolution_clock::now();
    if (!enabled) {
      return;
    }
    // 计算所有需要报告的时间点
    for (double t = start / double(fps) + 1; t <= total_duration; t += progress_interval * 60) {
      report_times.push_back(t);
    }
  }

  /**
   * @brief 输出报告
   *
   * @param elapsed_time 当前已处理到的视频时间（s）
   * @param frame_count 已经提取出来的图像数（有效的）
   */
  void report_progress(double elapsed_time, int frame_count) {
    if (!enabled) {
      return;
    }
    if (!report_times.empty() && elapsed_time >= report_times.front()) {
      auto now = chrono::high_resolution_clock::now();
      chrono::duration<double> processed_time = now - start_time;
      double percent = (elapsed_time * fps - start) / (end - start) * 100;
      ostringstream formatted_percent;
      formatted_percent << fixed << setprecision(2) << percent;
      cout << "\r" << std::string(80, ' '); // 清除当前行
      cout << "\r已处理 " << formatted_percent.str() << " % 的视频内容，已花费时间："
           << time_format(processed_time.count()) << "，已提取图片数：" << frame_count << flush;
      report_times.erase(report_times.begin()); // 移除已报告的时间点
    }
  }

  /**
   * @brief 输出处理完成后的统计结果。
   *
   * @param frame_count 最终输出的图片数量。
   */
  void report_result(int frame_count) {
    if (!enabled) {
      return;
    }
    auto now = chrono::high_resolution_clock::now();
    chrono::duration<double> total_time = now - start_time;
    cout << "\n处理总用时：" << time_format(total_time.count()) << "，输出图片数：" << frame_count
         << endl;
  }

  private:
  const double total_duration;                                  // 总时长
  const int fps;                                                // 帧率
  const int progress_interval;                                  // 进度报告间隔
  const int start;                                              // 提取开始帧
  const int end;                                                // 提取结束帧
  const bool enabled;                                           // 是否启用输出
  chrono::time_point<chrono::high_resolution_clock> start_time; // 开始时间
  vector<double> report_times;                                  // 存储所有报告的时间点

}; // class ProgressReporter

/**
 * @brief 计算图像的感知哈希值
 *
 * @param img 输入图像
 * @return size_t 计算得到的哈希值
 */
size_t calculate_pHash(const Mat& img) {
  // 就是感知哈希
  Mat resized;
  resize(img, resized, Size(32, 32));
  Mat gray;
  cvtColor(resized, gray, COLOR_BGR2GRAY);

  // 后面DCT函数需要浮点型的，所以在此转化一下
  Mat gray_float;
  gray.convertTo(gray_float, CV_64F);

  // 计算DCT
  Mat dct_result;
  dct(gray_float, dct_result);

  // 提取左上角8x8的DCT系数
  Mat dct_roi = dct_result(Rect(0, 0, 8, 8));
  double mean = cv::mean(dct_roi)[0];

  // 生成哈希值
  size_t hash = 0;
  for (int i = 0; i < dct_roi.rows; ++i) {
    for (int j = 0; j < dct_roi.cols; ++j) {
      // 如果DCT系数大于平均值，则设置对应的位为1
      hash <<= 1;
      if (dct_roi.at<double>(i, j) > mean) {
        hash |= 1;
      }
    }
  }

  return hash;
}

// 提取帧函数
/**
 * @brief 根据运行参数提取视频帧并输出到目标目录。
 *
 * @param options 已完成解析与校验的运行参数。
 * @return bool 提取成功返回 `true`，出现文件或视频信息错误时返回 `false`。
 */
bool extract_frames(const RunOptions& options) {
  // 打开视频文件，并且计算帧数等参数
  VideoCapture cap(options.input_file);

  if (!cap.isOpened()) {
    cerr << "无法打开视频文件: " << options.input_file << endl;
    return false;
  }

  int fps = cap.get(CAP_PROP_FPS);                  // 帧率
  int total_frames = cap.get(CAP_PROP_FRAME_COUNT); // 总帧数
  if (fps <= 0 || total_frames <= 0) {
    cerr << "无法读取有效的视频帧信息" << endl;
    return false;
  }
  double total_duration = total_frames / double(fps); // 总时长（s）

  int start_frame = (options.start_minutes <= 0) ? 0 : options.start_minutes * 60 * fps; // 起始帧
  int end_frame =
      (options.end_minutes <= 0) ? total_frames : min(options.end_minutes * 60 * fps, total_frames);
  if (start_frame >= total_frames) {
    cerr << "起点超出了视频时长" << endl;
    return false;
  }
  if (end_frame <= start_frame) {
    cerr << "目标片段为空，请检查起止时间" << endl;
    return false;
  }

  // 报告用，可能要改
  ProgressReporter progress_reporter(total_duration, fps, options.progress_interval, start_frame,
                                     end_frame, !options.quiet);
  vector<size_t> hash_list; // 存储哈希值
  // set<size_t> hash_set; // 存储哈希值
  int frame_count = 0; // 已经提取出来的图像数（有效的）
  int frame_index = start_frame;

  Mat frame;
  while (cap.isOpened()) {
    cap.set(CAP_PROP_POS_FRAMES, frame_index);
    if (!cap.read(frame))
      break;

    size_t img_hash = calculate_pHash(frame);

    bool similar = false;
    if (hash_list.size() >= HASH_WINDOW_SIZE) {
      for (int i = hash_list.size() - HASH_WINDOW_SIZE; i < hash_list.size(); ++i) {
        int hamming_distance =
            __builtin_popcountll(static_cast<unsigned long long>(hash_list[i] ^ img_hash));
        if (hamming_distance < options.threshold) {
          similar = true;
          break;
        }
      }
    }

    if (!similar) {
      double elapsed_time =
          cap.get(CAP_PROP_POS_FRAMES) / double(fps); // 当前已处理到的视频时间（s）
      string frame_path = options.output_folder + "/frame_" + to_string(int(elapsed_time / 60)) +
                          "min_" + to_string(frame_count) + ".jpg";
      imwrite(frame_path, frame);
      hash_list.push_back(img_hash);
      progress_reporter.report_progress(elapsed_time, frame_count);
      frame_count++;
    }

    if (cap.get(CAP_PROP_POS_FRAMES) >= end_frame)
      break;
    frame_index += options.frame_skip;
  }

  cap.release();
  progress_reporter.report_result(frame_count);
  return true;
}

/**
 * @brief 程序主入口，负责参数解析、模式分流与任务调度。
 *
 * @param argc 命令行参数个数。
 * @param argv 命令行参数数组。
 * @return int 成功返回 `0`，失败返回非 `0` 错误码。
 */
int main(int argc, char** argv) {
  system("chcp 65001");
  printf("Program version: %s\n", PROGRAM_VERSION);

  CliParseResult parse_result = parse_cli_arguments(argc, argv);
  if (!parse_result.error_message.empty()) {
    cerr << parse_result.error_message << endl << endl;
    print_cli_help(cerr);
    return 1;
  }

  if (parse_result.show_help) {
    print_cli_help(cout);
    return 0;
  }

  if (parse_result.show_version) {
    return 0;
  }

  const bool interactive_mode = (argc == 1) || parse_result.interactive;
  RunOptions options = interactive_mode ? prompt_for_options(parse_result.partial_options)
                                        : merge_with_defaults(parse_result.partial_options);
  const auto finish = [interactive_mode](int exit_code) {
    if (interactive_mode) {
      system("PAUSE");
    }
    return exit_code;
  };

  const string validation_error = validate_options(options);
  if (!validation_error.empty()) {
    cerr << "参数错误: " << validation_error << endl;
    return finish(1);
  }

  std::error_code error_code;
  if (!fs::exists(options.output_folder) &&
      !fs::create_directories(options.output_folder, error_code)) {
    cerr << "无法创建输出文件夹: " << options.output_folder << endl;
    if (error_code) {
      cerr << error_code.message() << endl;
    }
    return finish(1);
  }

  if (!extract_frames(options)) {
    return finish(1);
  }
  return finish(0);
}
