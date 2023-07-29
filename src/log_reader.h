#pragma once

#include <fstream>
#include <string>
#include <unordered_map>

#include <yaml-cpp/yaml.h>

#include "src/log_reader_exception.h"
#include "src/log_reader_types.h"

namespace ss {

class LogReader {
 public:
  LogReader(const std::string& path);

  // Not copyable.
  LogReader(const LogReader&) = delete;
  LogReader& operator=(const LogReader&) = delete;

  std::unordered_map<std::string, TypeBox> GetMessageTypes() const { return msg_types_; };
  void Load(const std::vector<Type *>& msgs);
  std::unordered_map<std::string, TypeBox> LoadAll();

 private:
  static inline BitfieldStruct ParseBitfield(std::string name, const YAML::Node& node);
  static inline Array ParseArray(const std::string& name, const YAML::Node& node,
                                 const std::unordered_map<std::string, TypeBox>& all_types);
  static inline Struct ParseStruct(std::string name, const YAML::Node& node,
                                   const std::unordered_map<std::string, TypeBox>& all_types);
  static inline TypeBox ParseEnum(std::string name, const YAML::Node& node);

  void ParseTypes();

  std::string path_;
  int binary_start_;
  YAML::Node msg_spec_;
  std::ifstream log_file_;
  std::unordered_map<std::string, TypeBox> msg_types_;
};

};  // namespace ss
