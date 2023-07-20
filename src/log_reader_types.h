#pragma once

#include <string>
#include <variant>
#include <vector>

#include "src/log_reader_exception.h"

namespace ss {

template <typename>
inline constexpr bool always_false_v = false;

template <typename T>
static inline T UnpackPrimitive(const uint8_t *data) {
  T value;

  if constexpr (sizeof(value) == 1) {
    uint8_t raw_value = data[0];
    memcpy(&value, &raw_value, sizeof(value));
  } else if constexpr (sizeof(value) == 2) {
    uint16_t raw_value =
        (static_cast<uint16_t>(data[0]) << 8) | (static_cast<uint16_t>(data[1]) << 0);
    memcpy(&value, &raw_value, sizeof(value));
  } else if constexpr (sizeof(value) == 4) {
    uint32_t raw_value =
        (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) |
        (static_cast<uint32_t>(data[2]) << 8) | (static_cast<uint32_t>(data[3]) << 0);
    memcpy(&value, &raw_value, sizeof(value));
  } else if constexpr (sizeof(value) == 8) {
    uint64_t raw_value =
        (static_cast<uint64_t>(data[0]) << 56) | (static_cast<uint64_t>(data[1]) << 48) |
        (static_cast<uint64_t>(data[2]) << 40) | (static_cast<uint64_t>(data[3]) << 32) |
        (static_cast<uint64_t>(data[4]) << 24) | (static_cast<uint64_t>(data[5]) << 16) |
        (static_cast<uint64_t>(data[6]) << 8) | (static_cast<uint64_t>(data[7]) << 0);
    memcpy(&value, &raw_value, sizeof(value));
  } else {
    static_assert(always_false_v<T>);
  }

  return value;
}

class LogData {
 public:
  enum class Kind {
    kPrimitive,
    kEnum,
    kArray,
    kStruct,
    kBitfieldStruct,
  };

  enum class PrimType {
    kUint8,
    kUint16,
    kUint32,
    kUint64,
    kInt8,
    kInt16,
    kInt32,
    kInt64,
    kBool,
    kFloat,
    kDouble,
  };

  static LogData Primitive(std::string type_name, PrimType data_type) {
    return LogData(Kind::kPrimitive, type_name, PrimitiveData{.type = data_type});
  }

  static LogData Enum(std::string type_name, std::vector<std::string> values) {
    PrimType prim_type;

    if (values.size() <= (1ULL << 7) - 1) {
      prim_type = PrimType::kInt8;
    } else if (values.size() <= (1ULL << 15) - 1) {
      prim_type = PrimType::kInt16;
    } else if (values.size() <= (1ULL << 31) - 1) {
      prim_type = PrimType::kInt32;
    } else if (values.size() <= (1ULL << 63) - 1) {
      prim_type = PrimType::kInt64;
    } else {
      throw LogParseException("Too many enum values.");
    }

    LogData data(Kind::kEnum, type_name, EnumData{.primitive = {.type = prim_type}, .values = values});

    return data;
  }

  static LogData BitfieldStruct(std::string type_name, std::vector<LogData> fields) {
    return LogData(Kind::kBitfieldStruct, type_name, BitfieldStructData{.fields = fields});
  }

  Kind GetKind() { return kind_; }

  std::string GetTypeName() { return type_name_; }

  std::string GetInstName() { return inst_name_; }

 private:
  struct PrimitiveData {
    PrimType type;
  };

  struct EnumData {
    PrimitiveData primitive;
    std::vector<std::string> values;
  };

  struct BitfieldStructData {
    std::vector<LogData> fields;
  };

  struct BitfieldFieldData {
    PrimitiveData primitive;
    int bit_offset;
    int bit_size;
  };

  using TypeData = std::variant<PrimitiveData, EnumData, BitfieldStructData>;

  LogData(Kind kind, std::string type_name, TypeData type_data)
      : kind_{kind}, type_name_{type_name}, type_data_{type_data} {}

  Kind kind_;
  std::string type_name_;
  std::string inst_name_;
  TypeData type_data_;
};

};  // namespace ss
