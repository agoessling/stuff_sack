#pragma once

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

  std::unordered_map<std::string, AnyType> GetMessageTypes();

 private:
  std::unordered_map<std::string, AnyType> GetAllTypes();

  std::string path_;
  int binary_start_;
  YAML::Node msg_spec_;
  std::unordered_map<std::string, AnyType> all_types_;
};

};  // namespace ss
