#include "src/log_reader.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <optional>
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

  log_file_.open(path, std::ios::in | std::ios::binary);
  if (!log_file_.is_open()) {
    throw std::system_error(errno, std::generic_category(), path);
  }

  std::vector<char> yaml_str(yaml_end + 1);
  log_file_.read(yaml_str.data(), yaml_str.size());
  if (log_file_.gcount() != static_cast<int>(yaml_str.size())) {
    throw LogParseException("Could not read YAML header from: " + path);
  }
  yaml_str.back() = 0;

  try {
    msg_spec_ = YAML::Load(yaml_str.data());
  } catch (const std::runtime_error& err) {
    throw LogParseException("Could not parse YAML header (" + path + "): " + err.what());
  }

  ParseTypes();
}

inline BitfieldStruct LogReader::ParseBitfield(std::string type_name, const YAML::Node& node) {
  BitfieldStruct s{std::move(type_name)};

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

inline Array LogReader::ParseArray(const std::string& inst_name, const YAML::Node& node,
                                   const std::unordered_map<std::string, TypeBox>& all_types) {
  Array a{inst_name};

  const YAML::Node& type_node = node[0];
  const int length = node[1].as<int>();

  for (int i = 0; i < length; ++i) {
    const std::string elem_name = inst_name + "[" + std::to_string(i) + "]";

    if (type_node.IsScalar()) {
      TypeBox elem_type = all_types.at(type_node.as<std::string>());
      elem_type->inst_name_ = elem_name;
      a.AddElem(*elem_type);
    } else if (type_node.IsSequence()) {
      a.AddElem(ParseArray(elem_name, type_node, all_types));
    } else {
      throw LogParseException("Unrecognized array description.");
    }
  }

  return a;
}

inline Struct LogReader::ParseStruct(std::string type_name, const YAML::Node& node,
                                     const std::unordered_map<std::string, TypeBox>& all_types) {
  Struct s{std::move(type_name)};

  const YAML::Node& fields = node["fields"];
  for (YAML::const_iterator list_it = fields.begin(); list_it != fields.end(); ++list_it) {
    for (YAML::const_iterator field_it = list_it->begin(); field_it != list_it->end(); ++field_it) {
      const std::string& field_name = field_it->first.as<std::string>();
      if (field_name.rfind("_", 0) == 0) {
        continue;
      }

      const YAML::Node& field_type_node = field_it->second;

      if (field_type_node.IsScalar()) {
        TypeBox field_type = all_types.at(field_type_node.as<std::string>());
        field_type->inst_name_ = field_name;
        s.AddField(*field_type);
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

inline TypeBox LogReader::ParseEnum(std::string type_name, const YAML::Node& node) {
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

  if (num_values <= (1ULL << 7) - 1) {
    Enum<int8_t> e{std::move(type_name)};
    e.SetValues(std::move(value_names));
    return e;
  } else if (num_values <= (1ULL << 15) - 1) {
    Enum<int16_t> e{std::move(type_name)};
    e.SetValues(std::move(value_names));
    return e;
  } else if (num_values <= (1ULL << 31) - 1) {
    Enum<int32_t> e{std::move(type_name)};
    e.SetValues(std::move(value_names));
    return e;
  } else if (num_values <= (1ULL << 63) - 1) {
    Enum<int64_t> e{std::move(type_name)};
    e.SetValues(std::move(value_names));
    return e;
  } else {
    throw LogParseException("Too many enum values.");
  }
}

void LogReader::ParseTypes() {
  std::unordered_map<std::string, TypeBox> all_types;
  all_types["uint8"] = Primitive<uint8_t>{"uint8"};
  all_types["uint16"] = Primitive<uint16_t>{"uint16"};
  all_types["uint32"] = Primitive<uint32_t>{"uint32"};
  all_types["uint64"] = Primitive<uint64_t>{"uint64"};
  all_types["int8"] = Primitive<int8_t>{"int8"};
  all_types["int16"] = Primitive<int16_t>{"int16"};
  all_types["int32"] = Primitive<int32_t>{"int32"};
  all_types["int64"] = Primitive<int64_t>{"int64"};
  all_types["bool"] = Primitive<bool>{"bool"};
  all_types["float"] = Primitive<float>{"float"};
  all_types["double"] = Primitive<double>{"double"};

  try {
    const YAML::Node& uid_map = msg_spec_["SsMessageUidMap"];

    for (YAML::const_iterator it = msg_spec_.begin(); it != msg_spec_.end(); ++it) {
      const std::string& name = it->first.as<std::string>();
      const YAML::Node& type_node = it->second;

      const YAML::Node& type_name_node = type_node["type"];
      if (!type_name_node) continue;

      const std::string& type_name = type_name_node.as<std::string>();
      if (type_name == "Struct" || type_name == "Message") {
        TypeBox type = ParseStruct(name, type_node, all_types);
        if (type_name == "Message") {
          type->SetMsgInfo(uid_map[name].as<uint32_t>(), 0);
          msg_types_[name] = type;
        }
        all_types[name] = type;
      } else if (type_name == "Enum") {
        all_types[name] = ParseEnum(name, type_node);
      } else if (type_name == "Bitfield") {
        all_types[name] = ParseBitfield(name, type_node);
      }
    }
  } catch (const std::exception& err) {
    throw LogParseException(err.what());
  }
}

class BufferedMsgReader {
 public:
  struct MsgInfo {
    uint32_t uid;
    uint16_t len;
    uint8_t *msg;
  };

  BufferedMsgReader(std::ifstream& log, size_t binary_start)
      : buf_(4096), index_{0}, buf_used_{0}, log_{log} {
    log_.seekg(binary_start);
    if (!log_) throw LogParseException("Malformed log");
  }

  std::optional<MsgInfo> GetMsg() {
    // If at the end of the buffer load more from file.
    if (BufRemainingLen() < kHeaderLen) {
      ShiftAndFill();

      if (BufRemainingLen() == 0) {
        return std::nullopt;
      }

      if (BufRemainingLen() < kHeaderLen) {
        throw LogParseException("Corrupted log end");
      }
    }

    MsgInfo info;
    info.uid = UnpackPrimitive<uint32_t>(&buf_[index_] + 0);
    info.len = UnpackPrimitive<uint16_t>(&buf_[index_] + 4);

    // Resize buffer for large messages to avoid continually aligning message to buffer start.
    if (4 * info.len > buf_.size()) {
      buf_.resize(4 * info.len);
    }

    if (info.len > BufRemainingLen()) {
      ShiftAndFill();
      if (info.len > BufRemainingLen()) throw LogParseException("Corrupted log end");
    }

    info.msg = &buf_[index_];
    index_ += info.len;

    return info;
  }

 private:
  static constexpr size_t kHeaderLen = 6;

  size_t BufRemainingLen() const { return buf_used_ - index_; }

  void ShiftAndFill() {
    const size_t remaining_len = BufRemainingLen();
    memmove(&buf_[0], &buf_[index_], remaining_len);
    log_.read(reinterpret_cast<char *>(&buf_[remaining_len]), buf_.size() - remaining_len);
    buf_used_ = log_.gcount() + remaining_len;
    index_ = 0;
  }

  std::vector<uint8_t> buf_;
  size_t index_;
  size_t buf_used_;
  std::ifstream& log_;
};

void LogReader::Load(const std::vector<Type *>& msgs) {
  std::unordered_map<uint32_t, std::vector<Type *>> unpack_lookup;
  for (Type *type : msgs) {
    unpack_lookup[type->msg_uid_].push_back(type);
  }

  BufferedMsgReader reader(log_file_, binary_start_);

  while (std::optional<BufferedMsgReader::MsgInfo> info = reader.GetMsg()) {
    auto lookup = unpack_lookup.find(info->uid);
    if (lookup == unpack_lookup.end()) continue;

    for (Type *type : lookup->second) {
      type->Unpack(info->msg);
    }
  }
}

std::unordered_map<std::string, TypeBox> LogReader::LoadAll() {
  std::unordered_map<std::string, TypeBox> msgs = GetMessageTypes();
  std::vector<Type *> read_list;
  for (auto& pair : msgs) {
    read_list.push_back(pair.second.get());
  }

  Load(read_list);
  return msgs;
}
