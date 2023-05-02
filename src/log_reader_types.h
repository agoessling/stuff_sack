#pragma once

#include <cstring>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "src/log_reader_exception.h"

namespace ss {

template <typename T>
class Box {
 public:
  Box(T&& obj) : ptr_(new T(std::move(obj))) {}
  Box(const T& obj) : ptr_(new T(obj)) {}

  Box(const Box& other) : Box(*other.ptr_) {}
  Box& operator=(const Box& other) {
    *ptr_ = *other.ptr_;
    return *this;
  }

  ~Box() = default;

  T& operator*() { return *ptr_; }
  const T& operator*() const { return *ptr_; }

  T *operator->() { return ptr_.get(); }
  const T *operator->() const { return ptr_.get(); }

  T *get() { return ptr_.get(); }
  const T *get() const { return ptr_.get(); }

 private:
  std::unique_ptr<T> ptr_;
};

template <typename>
inline constexpr bool always_false_v = false;

template <typename T>
static inline T UnpackPrimitive(const uint8_t *data) {
  T value;

  if constexpr (sizeof(value) == 1) {
    uint8_t raw_value = data[0];
    memcpy(value, raw_value, sizeof(value));
  } else if constexpr (sizeof(value) == 2) {
    uint16_t raw_value =
        (static_cast<uint16_t>(data[0]) << 8) | (static_cast<uint16_t>(data[1]) << 0);
    memcpy(value, raw_value, sizeof(value));
  } else if constexpr (sizeof(value) == 4) {
    uint32_t raw_value =
        (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) |
        (static_cast<uint32_t>(data[2]) << 8) | (static_cast<uint32_t>(data[3]) << 0);
    memcpy(value, raw_value, sizeof(value));
  } else if constexpr (sizeof(value) == 8) {
    uint64_t raw_value =
        (static_cast<uint64_t>(data[0]) << 56) | (static_cast<uint64_t>(data[1]) << 48) |
        (static_cast<uint64_t>(data[2]) << 40) | (static_cast<uint64_t>(data[3]) << 32) |
        (static_cast<uint64_t>(data[4]) << 24) | (static_cast<uint64_t>(data[5]) << 16) |
        (static_cast<uint64_t>(data[6]) << 8) | (static_cast<uint64_t>(data[7]) << 0);
    memcpy(value, raw_value, sizeof(value));
  } else {
    static_assert(always_false_v<T>);
  }

  return value;
}

class Element;

template <typename T>
class Primitive;

template <typename T>
class BitfieldPrimitive;

template <typename T>
class Enum;

class Struct;
class BitfieldStruct;
class Array;

using AnyType =
    std::variant<Primitive<uint8_t>, Primitive<uint16_t>, Primitive<uint32_t>, Primitive<uint64_t>,
                 Primitive<int8_t>, Primitive<int16_t>, Primitive<int32_t>, Primitive<int64_t>,
                 Primitive<bool>, Primitive<float>, Primitive<double>, BitfieldPrimitive<uint8_t>,
                 BitfieldPrimitive<uint16_t>, BitfieldPrimitive<uint32_t>,
                 BitfieldPrimitive<uint64_t>, Enum<int8_t>, Enum<int16_t>, Enum<int32_t>,
                 Enum<int64_t>, Struct, BitfieldStruct, Array>;

class Element {
 public:
  std::string name;
  std::string type_name;
  uint32_t msg_uid;
};

template <typename T>
class Primitive : public Element {
 public:
  static constexpr unsigned int packed_size = sizeof(T);

  std::vector<T> data;
};

template <typename T>
class BitfieldPrimitive : public Primitive<T> {
 public:
  int bit_offset;
  int bit_size;
};

template <typename T>
class Enum : public Element {
 public:
  static constexpr unsigned int packed_size = sizeof(T);

  std::vector<std::string> values;
  std::vector<T> data;
};

struct BitfieldOffsetVisitor {
  template <typename T>
  int operator()(const BitfieldPrimitive<T>& element) {
    return element.bit_offset;
  }

  template <typename T>
  int operator()(const T& element) {
    throw std::runtime_error("Only valid with Bitfield.");
    return 0;
  }
};

struct BitfieldSizeVisitor {
  template <typename T>
  int operator()(const BitfieldPrimitive<T>& element) {
    return element.bit_size;
  }

  template <typename T>
  int operator()(const T& element) {
    throw std::runtime_error("Only valid with Bitfield.");
    return 0;
  }
};

struct NameVisitor {
  template <typename T>
  const std::string& operator()(const T& element) {
    return element.name;
  }

  template <typename T>
  std::string& operator()(T& element) {
    return element.name;
  }
};

struct TypeVisitor {
  template <typename T>
  const std::string& operator()(const T& element) {
    return element.type_name;
  }

  template <typename T>
  std::string& operator()(T& element) {
    return element.type_name;
  }
};

struct MsgUidVisitor {
  template <typename T>
  const uint32_t& operator()(const T& element) {
    return element.msg_uid;
  }

  template <typename T>
  uint32_t& operator()(T& element) {
    return element.msg_uid;
  }
};

struct SetInstanceNameVisitor {
  const std::string new_name;

  template <typename T>
  void operator()(T& element) {
    element.type_name = std::move(element.name);
    element.name = new_name;
  }
};

struct PackedSizeVisitor {
  template <typename T>
  int operator()(const T& element) {
    return element.packed_size;
  }
};

class Struct : public Element {
 public:
  unsigned int packed_size = 0;
  std::vector<Box<AnyType>> fields;

  AnyType *operator[](const std::string& name);
  void AddField(AnyType&& type);
  void AddField(const AnyType& type);
};

class BitfieldStruct : public Struct {
 public:
  void AddBitfield(const std::string name, int bits);

 private:
  struct Bitfield {
    int offset;
    int size;
  };

  std::vector<Bitfield> bitfields_;
};

class Array : public Element {
 public:
  unsigned int packed_size = 0;
  std::vector<Box<AnyType>> elems;

  void AddElem(AnyType&& type);
  void AddElem(const AnyType& type);
};

inline AnyType *Struct::operator[](const std::string& name) {
  for (Box<AnyType>& field : fields) {
    if (name == std::visit(NameVisitor{}, *field)) {
      return field.get();
    }
  }
  return nullptr;
}

inline void Struct::AddField(AnyType&& type) {
  packed_size += std::visit(PackedSizeVisitor{}, type);
  fields.emplace_back(std::move(type));
  std::visit(MsgUidVisitor{}, *fields.back()) = msg_uid;
}

inline void Struct::AddField(const AnyType& type) {
  packed_size += std::visit(PackedSizeVisitor{}, type);
  fields.emplace_back(type);
  std::visit(MsgUidVisitor{}, *fields.back()) = msg_uid;
}

inline void BitfieldStruct::AddBitfield(const std::string name, int bits) {
  int offset = 0;
  if (!fields.empty()) {
    const int prev_offset = std::visit(BitfieldOffsetVisitor{}, *fields.back());
    const int prev_size = std::visit(BitfieldSizeVisitor{}, *fields.back());
    offset = prev_offset + prev_size;
  }

  const int packed_size_bits = offset + bits;
  if (packed_size_bits <= 8) {
    packed_size = 1;
  } else if (packed_size_bits <= 16) {
    packed_size = 2;
  } else if (packed_size_bits <= 32) {
    packed_size = 4;
  } else if (packed_size_bits <= 64) {
    packed_size = 8;
  } else {
    throw LogParseException("Bitfield struct too large.");
  }

  if (bits <= 8) {
    fields.emplace_back(BitfieldPrimitive<uint8_t>{{{name, "uint8"}}, offset, bits});
  } else if (bits <= 16) {
    fields.emplace_back(BitfieldPrimitive<uint16_t>{{{name, "uint16"}}, offset, bits});
  } else if (bits <= 32) {
    fields.emplace_back(BitfieldPrimitive<uint32_t>{{{name, "uint32"}}, offset, bits});
  } else if (bits <= 64) {
    fields.emplace_back(BitfieldPrimitive<uint64_t>{{{name, "uint64"}}, offset, bits});
  } else {
    throw LogParseException("Bitfield field too long.");
  }

  std::visit(MsgUidVisitor{}, *fields.back()) = msg_uid;
}

inline void Array::AddElem(AnyType&& type) {
  packed_size += std::visit(PackedSizeVisitor{}, type);
  elems.emplace_back(std::move(type));
  std::visit(MsgUidVisitor{}, *elems.back()) = msg_uid;
}

inline void Array::AddElem(const AnyType& type) {
  packed_size += std::visit(PackedSizeVisitor{}, type);
  elems.emplace_back(type);
  std::visit(MsgUidVisitor{}, *elems.back()) = msg_uid;
}

};  // namespace ss
