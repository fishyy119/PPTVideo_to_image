#include "ArgumentParser.hpp"

#include <functional>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace std;

namespace {

using OptionAction = function<bool(const string&, CliParseResult&, string&)>;

struct OptionSpec {
  string long_name;
  char short_name;
  bool requires_value;
  string value_name;
  string description;
  OptionAction apply;
};

/**
 * @brief 将字符串解析为整数参数。
 *
 * @param option_name 当前参数名，用于拼装错误信息。
 * @param value 待解析的字符串值。
 * @param target 解析成功后写入的目标变量。
 * @param error 解析失败时写入的错误信息。
 * @return bool 解析成功返回 `true`，失败返回 `false`。
 */
bool parse_int_argument(const string& option_name, const string& value, int& target,
                        string& error) {
  try {
    size_t parsed_length = 0;
    const int parsed_value = stoi(value, &parsed_length);
    if (parsed_length != value.size()) {
      throw invalid_argument("contains trailing characters");
    }
    target = parsed_value;
    return true;
  } catch (const exception&) {
    error = "参数 --" + option_name + " 需要整数值";
    return false;
  }
}

/**
 * @brief 返回程序支持的命令行选项描述表。
 *
 * @return const vector<OptionSpec>& 包含参数定义及处理逻辑的静态选项表。
 */
const vector<OptionSpec>& option_specs() {
  static const vector<OptionSpec> specs = {
      {"input", 'i', true, "PATH", "输入视频文件路径",
       [](const string& value, CliParseResult& result, string&) {
         result.partial_options.input_file = value;
         return true;
       }},
      {"output", 'o', true, "PATH", "输出文件夹路径",
       [](const string& value, CliParseResult& result, string&) {
         result.partial_options.output_folder = value;
         return true;
       }},
      {"start", 's', true, "MIN", "起点分钟数",
       [](const string& value, CliParseResult& result, string& error) {
         int parsed_value = 0;
         if (!parse_int_argument("start", value, parsed_value, error)) {
           return false;
         }
         result.partial_options.start_minutes = parsed_value;
         return true;
       }},
      {"end", 'e', true, "MIN", "终点分钟数，使用 -1 表示到视频结尾",
       [](const string& value, CliParseResult& result, string& error) {
         int parsed_value = 0;
         if (!parse_int_argument("end", value, parsed_value, error)) {
           return false;
         }
         result.partial_options.end_minutes = parsed_value;
         return true;
       }},
      {"frame-skip", 'f', true, "N", "跳帧检测值",
       [](const string& value, CliParseResult& result, string& error) {
         int parsed_value = 0;
         if (!parse_int_argument("frame-skip", value, parsed_value, error)) {
           return false;
         }
         result.partial_options.frame_skip = parsed_value;
         return true;
       }},
      {"progress-interval", 'p', true, "MIN", "进度提示间隔（分钟）",
       [](const string& value, CliParseResult& result, string& error) {
         int parsed_value = 0;
         if (!parse_int_argument("progress-interval", value, parsed_value, error)) {
           return false;
         }
         result.partial_options.progress_interval = parsed_value;
         return true;
       }},
      {"threshold", 't', true, "N", "相似度比较阈值",
       [](const string& value, CliParseResult& result, string& error) {
         int parsed_value = 0;
         if (!parse_int_argument("threshold", value, parsed_value, error)) {
           return false;
         }
         result.partial_options.threshold = parsed_value;
         return true;
       }},
      {"quiet", 'q', false, "", "安静启动，禁用进度输出",
       [](const string&, CliParseResult& result, string&) {
         result.partial_options.quiet = true;
         return true;
       }},
      {"interactive", '\0', false, "", "即使传入参数，也进入交互式确认",
       [](const string&, CliParseResult& result, string&) {
         result.interactive = true;
         return true;
       }},
      {"help", 'h', false, "", "显示帮助信息",
       [](const string&, CliParseResult& result, string&) {
         result.show_help = true;
         return true;
       }},
      {"version", 'v', false, "", "显示版本号",
       [](const string&, CliParseResult& result, string&) {
         result.show_version = true;
         return true;
       }},
  };
  return specs;
}

/**
 * @brief 按长参数名查找选项描述。
 *
 * @param name 长参数名，不含前缀 `--`。
 * @return const OptionSpec* 找到时返回对应描述，未找到时返回 `nullptr`。
 */
const OptionSpec* find_long_option(const string& name) {
  for (const auto& spec : option_specs()) {
    if (spec.long_name == name) {
      return &spec;
    }
  }
  return nullptr;
}

/**
 * @brief 按短参数名查找选项描述。
 *
 * @param name 短参数字符，不含前缀 `-`。
 * @return const OptionSpec* 找到时返回对应描述，未找到时返回 `nullptr`。
 */
const OptionSpec* find_short_option(char name) {
  for (const auto& spec : option_specs()) {
    if (spec.short_name == name) {
      return &spec;
    }
  }
  return nullptr;
}

/**
 * @brief 执行某个选项对应的赋值逻辑。
 *
 * @param spec 选项描述。
 * @param value 当前选项的参数值。
 * @param result 累积的解析结果。
 * @return bool 应用成功返回 `true`，失败时会写入错误信息并返回 `false`。
 */
bool apply_option(const OptionSpec& spec, const string& value, CliParseResult& result) {
  string error;
  if (!spec.apply(value, result, error)) {
    result.error_message = error;
    return false;
  }
  return true;
}

/**
 * @brief 生成用于报错的参数标签。
 *
 * @param spec 选项描述。
 * @return string 形如 `--input` 的长参数标签。
 */
string option_label(const OptionSpec& spec) {
  return "--" + spec.long_name;
}

/**
 * @brief 判断一个命令行片段是否是已知选项。
 *
 * @param argument 待判断的命令行片段。
 * @return bool 片段是已知长选项或短选项时返回 `true`。
 */
bool is_known_option_token(const string& argument) {
  if (argument.rfind("--", 0) == 0) {
    const string option_text = argument.substr(2);
    const size_t equals_pos = option_text.find('=');
    const string option_name =
        equals_pos == string::npos ? option_text : option_text.substr(0, equals_pos);
    return find_long_option(option_name) != nullptr;
  }

  if (argument.rfind("-", 0) == 0 && argument.size() >= 2) {
    return find_short_option(argument[1]) != nullptr;
  }

  return false;
}

/**
 * @brief 读取需要值的选项后面的独立参数值。
 *
 * @param argc 命令行参数个数。
 * @param argv 命令行参数数组。
 * @param index 当前解析位置，成功读取时会推进到值的位置。
 * @param spec 当前选项描述。
 * @param result 累积的解析结果，失败时写入错误信息。
 * @param option_value 成功读取时写入参数值。
 * @return bool 成功读取返回 `true`，缺值时返回 `false`。
 */
bool consume_next_value(int argc, char** argv, int& index, const OptionSpec& spec,
                        CliParseResult& result, string& option_value) {
  if (index + 1 >= argc) {
    result.error_message = option_label(spec) + " 缺少参数值";
    return false;
  }

  const string next_argument = argv[index + 1];
  if (is_known_option_token(next_argument)) {
    result.error_message = option_label(spec) + " 缺少参数值";
    return false;
  }

  option_value = argv[++index];
  return true;
}

} // namespace

CliParseResult parse_cli_arguments(int argc, char** argv) {
  CliParseResult result;

  for (int index = 1; index < argc; ++index) {
    const string argument = argv[index];
    if (argument.rfind("--", 0) == 0) {
      const string option_text = argument.substr(2);
      const size_t equals_pos = option_text.find('=');
      const bool has_inline_value = equals_pos != string::npos;
      const string option_name = has_inline_value ? option_text.substr(0, equals_pos) : option_text;
      string option_value = has_inline_value ? option_text.substr(equals_pos + 1) : "";

      const OptionSpec* spec = find_long_option(option_name);
      if (spec == nullptr) {
        result.error_message = "未知参数: --" + option_name;
        return result;
      }

      if (spec->requires_value) {
        if (!has_inline_value) {
          if (!consume_next_value(argc, argv, index, *spec, result, option_value)) {
            return result;
          }
        }
      } else if (has_inline_value) {
        result.error_message = option_label(*spec) + " 不接受参数值";
        return result;
      }

      if (!apply_option(*spec, option_value, result)) {
        return result;
      }
      continue;
    }

    if (argument.rfind("-", 0) == 0 && argument.size() >= 2) {
      const char option_name = argument[1];
      const OptionSpec* spec = find_short_option(option_name);
      if (spec == nullptr) {
        result.error_message = "未知参数: " + argument;
        return result;
      }

      string option_value;
      if (spec->requires_value) {
        if (argument.size() > 2) {
          if (argument[2] != '=') {
            result.error_message = "短参数格式错误: " + argument;
            return result;
          }
          option_value = argument.substr(3);
        } else {
          if (!consume_next_value(argc, argv, index, *spec, result, option_value)) {
            return result;
          }
        }
      } else if (argument.size() > 2) {
        result.error_message = "短参数格式错误: " + argument;
        return result;
      }

      if (!apply_option(*spec, option_value, result)) {
        return result;
      }
      continue;
    }

    result.error_message = "不支持的位置参数: " + argument;
    return result;
  }

  return result;
}

void print_cli_help(std::ostream& out) {
  out << "用法:\n";
  out << "  " << left << setw(26) << "PV2i.exe" << "交互式运行\n";
  out << "  " << left << setw(26) << "PV2i.exe [选项]" << "非交互式运行\n\n";
  out << "选项:\n";

  for (const auto& spec : option_specs()) {
    ostringstream label;
    label << "  ";
    if (spec.short_name != '\0') {
      label << "-" << spec.short_name << ", ";
    } else {
      label << "    ";
    }
    label << "--" << spec.long_name;
    if (spec.requires_value) {
      label << " <" << spec.value_name << ">";
    }

    out << left << setw(34) << label.str() << spec.description << '\n';
  }

  out << "\n示例:\n";
  out << "  PV2i.exe --input 1.mp4 --output out --start 0 --end 10 --frame-skip 30 "
         "--progress-interval 5 "
         "--threshold 4\n";
  out << "  PV2i.exe --quiet --input 1.mp4 --output out\n";
  out << "  PV2i.exe --interactive --input 1.mp4\n";
}
