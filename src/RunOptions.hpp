#pragma once

#include <optional>
#include <string>

struct PartialRunOptions {
  std::optional<std::string> input_file;
  std::optional<std::string> output_folder;
  std::optional<int> start_minutes;
  std::optional<int> end_minutes;
  std::optional<int> frame_skip;
  std::optional<int> progress_interval;
  std::optional<int> threshold;
  std::optional<bool> quiet;
};

struct RunOptions {
  std::string input_file;
  std::string output_folder;
  int start_minutes;
  int end_minutes;
  int frame_skip;
  int progress_interval;
  int threshold;
  bool quiet;
};

/**
 * @brief 创建一组默认运行参数。
 *
 * @return RunOptions 包含默认输入、默认输出目录名和默认处理参数的运行配置。
 */
RunOptions make_default_run_options();

/**
 * @brief 将部分参数与默认值合并，得到可执行的完整参数。
 *
 * @param partial 用户通过命令行或其他来源提供的部分参数。
 * @return RunOptions 使用默认值补全后的完整运行参数。
 */
RunOptions merge_with_defaults(const PartialRunOptions& partial);

/**
 * @brief 以给定参数为初始值，交互式补全并返回完整运行参数。
 *
 * @param seed 作为提示默认值使用的初始参数集合。
 * @return RunOptions 用户确认后的完整运行参数。
 */
RunOptions prompt_for_options(const PartialRunOptions& seed);

/**
 * @brief 校验运行参数是否合法。
 *
 * @param options 待校验的完整运行参数。
 * @return std::string 校验失败时返回错误信息，成功时返回空字符串。
 */
std::string validate_options(const RunOptions& options);

/**
 * @brief 生成默认输出目录名。
 *
 * @return std::string 格式为 `output_MMDD_HHmmss` 的目录名。
 */
std::string get_default_output_folder_name();
