#include "src/log_reader.h"

#include <cerrno>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "src/log_reader_types.h"

extern "C" {
#include "src/logging.h"
}

using namespace ss;

LogReader::LogReader(const std::string& path) {
  path_ = path;

  FILE *const f = fopen(path.c_str(), "rb");
  if (!f) {
    throw std::system_error(errno, std::generic_category(), path);
  }

  binary_start_ = SsFindLogDelimiter(f);

  fclose(f);

  if (binary_start_ < static_cast<int>(sizeof(kSsLogDelimiter)) - 1) {
    throw LogParseException("Could not find log delimiter in: " + path);
  }
  const int yaml_end = binary_start_ - static_cast<int>(sizeof(kSsLogDelimiter)) + 1;

  std::ifstream log_file(path, std::ios::in | std::ios::binary);
  if (!log_file.is_open()) {
    throw std::system_error(errno, std::generic_category(), path);
  }

  std::vector<char> yaml_str(yaml_end + 1);
  log_file.read(yaml_str.data(), yaml_str.size());
  if (log_file.gcount() != static_cast<int>(yaml_str.size())) {
    throw LogParseException("Could not read YAML header from: " + path);
  }
  yaml_str.back() = 0;

  try {
    msg_spec_ = YAML::Load(yaml_str.data());
  } catch (const std::runtime_error& err) {
    throw LogParseException("Could not parse YAML header (" + path + "): " + err.what());
  }

  all_types_ = GetAllTypes();
}

static inline AnyType ParseBitfield(const std::string& name, const YAML::Node& node) {
  BitfieldStruct s{};
  s.name = name;
  s.type_name = node["type"].as<std::string>();

  const YAML::Node& fields = node["fields"];
  for (YAML::const_iterator list_it = fields.begin(); list_it != fields.end(); ++list_it) {
    for (YAML::const_iterator field_it = list_it->begin(); field_it != list_it->end(); ++field_it) {
      const std::string& field_name = field_it->first.as<std::string>();
      if (field_name.rfind("_", 0) == 0) {
        continue;
      }

      const int field_bits = field_it->second.as<int>();
      s.AddBitfield(field_name, field_bits);
      break;
    }
  }

  return s;
}

static inline AnyType ParseArray(const std::string& name, const YAML::Node& node,
                                 const std::unordered_map<std::string, AnyType>& all_types) {
  Array a{{name}};

  const YAML::Node& type_node = node[0];
  const int length = node[1].as<int>();

  for (int i = 0; i < length; ++i) {
    std::string elem_name = name + "[" + std::to_string(i) + "]";

    AnyType type;
    if (type_node.IsScalar()) {
      AnyType elem_type = all_types.at(type_node.as<std::string>());
      std::visit(SetInstanceNameVisitor{elem_name}, elem_type);
      a.AddElem(std::move(elem_type));
    } else if (type_node.IsSequence()) {
      a.AddElem(ParseArray(elem_name, type_node, all_types));
    } else {
      throw LogParseException("Unrecognized array description.");
    }
  }

  a.type_name = std::visit(TypeVisitor{}, *a.elems[0]);

  return a;
}

static inline AnyType ParseStruct(const std::string& name, const YAML::Node& node,
                                  const std::unordered_map<std::string, AnyType>& all_types,
                                  uint32_t msg_uid) {
  Struct s{{name, node["type"].as<std::string>(), msg_uid}};

  const YAML::Node& fields = node["fields"];
  for (YAML::const_iterator list_it = fields.begin(); list_it != fields.end(); ++list_it) {
    for (YAML::const_iterator field_it = list_it->begin(); field_it != list_it->end(); ++field_it) {
      const std::string& field_name = field_it->first.as<std::string>();
      if (field_name.rfind("_", 0) == 0) {
        continue;
      }

      const YAML::Node& field_type_node = field_it->second;

      if (field_type_node.IsScalar()) {
        AnyType field_type = all_types.at(field_type_node.as<std::string>());
        std::visit(SetInstanceNameVisitor{field_name}, field_type);
        s.AddField(std::move(field_type));
      } else if (field_type_node.IsSequence()) {
        s.AddField(ParseArray(field_name, field_type_node, all_types));
      } else {
        throw LogParseException("Unrecognized field description.");
      }

      break;
    }
  }

  return s;
}

static inline AnyType ParseEnum(const std::string& name, const YAML::Node& node) {
  const YAML::Node& values = node["values"];

  std::vector<std::string> value_names;
  for (YAML::const_iterator list_it = values.begin(); list_it != values.end(); ++list_it) {
    for (YAML::const_iterator val_it = list_it->begin(); val_it != list_it->end(); ++val_it) {
      const std::string& val_name = val_it->first.as<std::string>();
      if (val_name.rfind("_", 0) == 0) {
        continue;
      }

      value_names.push_back(val_name);
      break;
    }
  }

  const uint64_t num_values = values.size();

  AnyType type;
  if (num_values <= (1ULL << 7) - 1) {
    Enum<int8_t> e{{name, node["type"].as<std::string>()}};
    e.values = std::move(value_names);
    type = std::move(e);
  } else if (num_values <= (1ULL << 15) - 1) {
    Enum<int16_t> e{{name, node["type"].as<std::string>()}};
    e.values = std::move(value_names);
    type = std::move(e);
  } else if (num_values <= (1ULL << 31) - 1) {
    Enum<int32_t> e{{name, node["type"].as<std::string>()}};
    e.values = std::move(value_names);
    type = std::move(e);
  } else if (num_values <= (1ULL << 63) - 1) {
    Enum<int64_t> e{{name, node["type"].as<std::string>()}};
    e.values = std::move(value_names);
    type = std::move(e);
  } else {
    throw LogParseException("Too many enum values.");
  }

  return type;
}

std::unordered_map<std::string, AnyType> LogReader::GetAllTypes() {
  std::unordered_map<std::string, AnyType> all_types;
  all_types["uint8"] = Primitive<uint8_t>{{"uint8", "Primitive"}};
  all_types["uint16"] = Primitive<uint16_t>{{"uint16", "Primitive"}};
  all_types["uint32"] = Primitive<uint32_t>{{"uint32", "Primitive"}};
  all_types["uint64"] = Primitive<uint64_t>{{"uint64", "Primitive"}};
  all_types["int8"] = Primitive<int8_t>{{"int8", "Primitive"}};
  all_types["int16"] = Primitive<int16_t>{{"int16", "Primitive"}};
  all_types["int32"] = Primitive<int32_t>{{"int32", "Primitive"}};
  all_types["int64"] = Primitive<int64_t>{{"int64", "Primitive"}};
  all_types["bool"] = Primitive<bool>{{"bool", "Primitive"}};
  all_types["float"] = Primitive<float>{{"float", "Primitive"}};
  all_types["double"] = Primitive<double>{{"double", "Primitive"}};

  try {
    const YAML::Node& uid_map = msg_spec_["SsMessageUidMap"];

    for (YAML::const_iterator it = msg_spec_.begin(); it != msg_spec_.end(); ++it) {
      const std::string& name = it->first.as<std::string>();
      const YAML::Node& type_node = it->second;

      const YAML::Node& type_name_node = type_node["type"];
      if (!type_name_node) continue;

      const std::string& type_name = type_name_node.as<std::string>();
      if (type_name == "Struct" || type_name == "Message") {
        uint32_t msg_uid = 0;
        if (type_name == "Message") {
          msg_uid = uid_map[name].as<uint32_t>();
        }

        all_types[name] = ParseStruct(name, type_node, all_types, msg_uid);
      } else if (type_name == "Enum") {
        all_types[name] = ParseEnum(name, type_node);
      } else if (type_name == "Bitfield") {
        all_types[name] = ParseBitfield(name, type_node);
      }
    }
  } catch (const std::exception& err) {
    throw LogParseException(err.what());
  }

  return all_types;
}

std::unordered_map<std::string, AnyType> LogReader::GetMessageTypes() {
  std::unordered_map<std::string, AnyType> msgs;

  for (const auto& [name, type] : all_types_) {
    if (std::visit(TypeVisitor{}, type) == "Message") {
      msgs[name] = type;
    }
  }

  return msgs;
}
