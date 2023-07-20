#pragma once

#include <fstream>
#include <string>
#include <unordered_map>

#include <yaml-cpp/yaml.h>

#include "src/log_reader_types.h"
#include "src/log_reader_exception.h"

namespace ss {

class LogReader {
 public:
  LogReader(const std::string& path);

  // Not copyable.
  LogReader(const LogReader&) = delete;
  LogReader& operator=(const LogReader&) = delete;

  std::unordered_map<std::string, LogData> GetMessageTypes();
  void Load(const std::vector<LogData *>& msgs);
  std::unordered_map<std::string, LogData> LoadAll();

 private:
  std::unordered_map<std::string, LogData> GetAllTypes();

  std::string path_;
  int binary_start_;
  YAML::Node msg_spec_;
  std::ifstream log_file_;
  std::unordered_map<std::string, LogData> all_types_;
};

};  // namespace ss
