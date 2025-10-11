#pragma once

#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Config {

// 配置项类型枚举
enum class ConfigType { INT, FLOAT, STRING };

// 单个配置项
struct ConfigItem {
  std::string name;
  ConfigType type;
  json defaultValue;

  ConfigItem(std::string name, ConfigType type, json defaultValue)
      : name(std::move(name)), type(type), defaultValue(std::move(defaultValue)) {}
};

std::vector<ConfigItem> defaultConfig = {ConfigItem("frame_skip", ConfigType::INT, 30),
                                         ConfigItem("progress_interval", ConfigType::INT, 5),
                                         ConfigItem("threshold", ConfigType::INT, 4)};

// 配置管理类
class ConfigManager {
  public:
  // 存储配置项（键值对：名称 -> 配置项值）
  std::unordered_map<std::string, json> configValues;
  int FRAME_SKIP;
  int PROGRESS_INTERVAL;
  int HASH_THRESHOLD;

  private:
  std::string configFilePath;
  json config;

  public:
  ConfigManager(const std::string& configFile) : configFilePath(configFile) {
    if (!loadConfig()) {
      std::cout << "未找到设置文件，已自动生成。\n";
      createDefaultConfig();
    }
  }

  // 获取配置项的值
  json getConfigValue(const std::string& key) {
    return configValues[key];
  }

  // 打印当前的配置参数
  void printConfig() const {
    for (const auto& item : configValues) {
      std::cout << item.first << ": " << item.second << "\n";
    }
  }

  private:
  // 加载配置文件
  bool loadConfig() {
    std::ifstream configFile(configFilePath);
    if (configFile.is_open()) {
      try {
        json j;
        configFile >> j;

        // 验证并解析配置项
        if (validateConfig(j)) {
          // 解析配置项并存储
          for (const auto& item : defaultConfig) {
            std::string key = item.name;
            if (j.contains(key)) {
              configValues[key] = j[key];
            } else {
              configValues[key] = item.defaultValue;
              std::cout << "d" << std::endl;
            }
          }
          return true;
        } else {
          std::cerr << "Config file format is incorrect.\n";
          return false;
        }
      } catch (const std::exception& e) {
        std::cerr << "Error reading config file: " << e.what() << "\n";
        return false;
      }
    }
    return false;
  }

  // 创建默认配置文件
  void createDefaultConfig() {
    json configJson;
    for (const auto& item : defaultConfig) {
      configJson[item.name] = item.defaultValue;
    }

    // 将 JSON 数据写入文件
    std::ofstream outFile(configFilePath);
    if (outFile.is_open()) {
      outFile << configJson.dump(4); // 格式化为 4 个空格的 JSON 输出
      outFile.close();
      std::cout << "默认配置已保存到" << configFilePath << std::endl;
    } else {
      std::cerr << "文件创建失败: " << configFilePath << std::endl;
    }
  }

  // 验证配置项是否完整
  bool validateConfig(const json& j) const {
    for (const auto& item : defaultConfig) {
      std::string key = item.name;
      if (!j.contains(key)) {
        std::cerr << "Missing config key: " << key << "\n";
        return false;
      }
      // 类型验证
      if (!validateType(j[key], item.type)) {
        std::cerr << "Invalid type for config key: " << key << "\n";
        return false;
      }
    }
    return true;
  }

  // 验证类型是否匹配
  bool validateType(const json& j, ConfigType type) const {
    switch (type) {
    case ConfigType::INT:
      return j.is_number_integer();
    case ConfigType::FLOAT:
      return j.is_number_float();
    case ConfigType::STRING:
      return j.is_string();
    default:
      return false;
    }
  }
};

}; // namespace Config
