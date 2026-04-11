#pragma once

#include <iosfwd>
#include <string>

#include "RunOptions.hpp"

struct CliParseResult {
  PartialRunOptions partial_options;
  bool show_help = false;
  bool show_version = false;
  bool interactive = false;
  std::string error_message;
};

/**
 * @brief 解析命令行参数。
 *
 * @param argc 命令行参数个数。
 * @param argv 命令行参数数组。
 * @return CliParseResult 解析结果，包含参数值、模式标记与错误信息。
 */
CliParseResult parse_cli_arguments(int argc, char** argv);

/**
 * @brief 输出命令行帮助信息。
 *
 * @param out 输出目标流。
 */
void print_cli_help(std::ostream& out);
