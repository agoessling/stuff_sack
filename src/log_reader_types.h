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

class Element {
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

  static Element Primitive(std::string type_name, PrimType data_type) {
    return Element(Kind::kPrimitive, type_name, PrimitiveInfo{.type = data_type});
  }

  static Element Enum(std::string type_name, std::vector<std::string> values) {
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

    Element data(Kind::kEnum, type_name,
                 EnumInfo{.primitive = {.type = prim_type}, .values = values});

    return data;
  }

  static Element BitfieldStruct(std::string type_name, std::vector<Element> fields) {
    return Element(Kind::kBitfieldStruct, type_name, BitfieldStructInfo{.fields = fields});
  }

  Kind GetKind() const { return kind_; }
  std::string GetTypeName() const { return type_name_; }
  std::string GetInstName() const { return inst_name_; }

  void AddBitfield(std::string name, int bits) {
    BitfieldStructInfo& info
  }

 private:
  using PrimData = std::variant<std::vector<uint8_t>, std::vector<uint16_t>, std::vector<uint32_t>,
                               std::vector<uint64_t>, std::vector<int8_t>, std::vector<int16_t>,
                               std::vector<int32_t>, std::vector<int64_t>, std::vector<bool>,
                               std::vector<float>, std::vector<double>>;

  struct PrimitiveInfo {
    PrimType type;
    PrimData data;
  };

  struct EnumInfo {
    PrimitiveInfo primitive;
    std::vector<std::string> values;
  };

  struct BitfieldFieldInfo {
    PrimitiveInfo primitive;
    int bit_offset;
    int bit_size;
  };

  struct BitfieldStructInfo {
    std::vector<Element> fields;
  };

  using TypeInfo = std::variant<PrimitiveInfo, EnumInfo, BitfieldStructInfo>;

  static constexpr size_t SizeFromType(PrimType type) {
    switch (type) {
      case PrimType::kUint8:
      case PrimType::kInt8:
      case PrimType::kBool:
        return 1;
      case PrimType::kUint16:
      case PrimType::kInt16:
        return 2;
      case PrimType::kUint32:
      case PrimType::kInt32:
      case PrimType::kFloat:
        return 4;
      case PrimType::kUint64:
      case PrimType::kInt64:
      case PrimType::kDouble:
        return 8;
    }
  }

  Element(Kind kind, std::string type_name, TypeInfo type_info)
      : kind_{kind}, type_name_{type_name}, type_info_{type_info} {}

  Kind kind_;
  std::string type_name_;
  std::string inst_name_;
  TypeInfo type_info_;
  size_t packed_size_;
};

};  // namespace ss
