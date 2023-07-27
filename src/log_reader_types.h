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
  Element() = default;
  Element(std::string type_name, std::string inst_name)
      : type_name_{std::move(type_name)}, inst_name_{std::move(inst_name)} {}
  Element(std::string type_name, std::string inst_name, uint32_t msg_uid)
      : type_name_{std::move(type_name)}, inst_name_{std::move(inst_name)}, msg_uid_{msg_uid} {}

  void SetMsgUid(uint32_t uid) { msg_uid_ = uid; }
  uint32_t GetMsgUid() const { return msg_uid_; }

  void SetTypeName(std::string type_name) { type_name_ = std::move(type_name); }
  std::string GetTypeName() const { return type_name_; }

  void SetInstName(std::string inst_name) { inst_name_ = std::move(inst_name); }
  std::string GetInstName() const { return inst_name_; }

  void SetMsgOffset(size_t offset) { msg_offset_ = offset; }
  size_t GetMsgOffset() { return msg_offset_; }

 private:
  std::string type_name_;
  std::string inst_name_;
  uint32_t msg_uid_;
  size_t msg_offset_;
};

template <typename T>
class Primitive : public Element {
 public:
  using Element::Element;

  static constexpr unsigned int PackedSize() { return sizeof(T); }
  void Unpack(const uint8_t *msg) { data_.push_back(UnpackPrimitive<T>(msg + GetMsgOffset())); }

 protected:
  std::vector<T> data_;
};

template <typename T>
class BitfieldPrimitive : public Primitive<T> {
 public:
  using Primitive<T>::Primitive;

  BitfieldPrimitive(std::string type_name, std::string inst_name, unsigned int packed_size,
                    int bit_offset, int bit_size)
      : Primitive<T>(type_name, inst_name),
        packed_size_{packed_size},
        bit_offset_{bit_offset},
        bit_size_{bit_size} {}

  int GetBitOffset() const { return bit_offset_; }
  int GetBitSize() const { return bit_size_; }

  void Unpack(const uint8_t *msg) {
    if (packed_size_ <= 1) {
      const uint8_t raw_data = UnpackPrimitive<uint8_t>(msg + this->GetMsgOffset());
      const uint8_t mask = (1 << bit_size_) - 1;
      this->data_.push_back((raw_data >> bit_offset_) & mask);
    }
  }

 private:
  unsigned int packed_size_;
  int bit_offset_;
  int bit_size_;
};

template <typename T>
class Enum : public Primitive<T> {
 public:
  using Primitive<T>::Primitive;

  Enum(std::string type_name, std::string inst_name, std::vector<std::string> values)
      : Primitive<T>(type_name, inst_name), values_{std::move(values)} {}

 private:
  std::vector<std::string> values_;
};

struct GetBitOffsetVisitor {
  template <typename T>
  int operator()(const BitfieldPrimitive<T>& element) {
    return element.GetBitOffset();
  }

  template <typename T>
  int operator()(const T& element) {
    throw std::runtime_error("Only valid with Bitfield.");
    return 0;
  }
};

struct GetBitSizeVisitor {
  template <typename T>
  int operator()(const BitfieldPrimitive<T>& element) {
    return element.GetBitSize();
  }

  template <typename T>
  int operator()(const T& element) {
    throw std::runtime_error("Only valid with Bitfield.");
    return 0;
  }
};

struct GetInstNameVisitor {
  template <typename T>
  std::string operator()(const T& element) {
    return element.GetInstName();
  }
};

struct GetTypeNameVisitor {
  template <typename T>
  std::string operator()(const T& element) {
    return element.GetTypeName();
  }
};

struct SetMsgUidVisitor {
  const uint32_t uid;

  template <typename T>
  void operator()(T& element) {
    element.SetMsgUid(uid);
  }
};

struct GetMsgUidVisitor {
  template <typename T>
  uint32_t operator()(const T& element) {
    return element.GetMsgUid();
  }
};

struct SetInstNameVisitor {
  const std::string name;

  template <typename T>
  void operator()(T& element) {
    element.SetInstName(name);
  }
};

struct SetTypeNameVisitor {
  const std::string name;

  template <typename T>
  void operator()(T& element) {
    element.SetTypeName(name);
  }
};

struct PackedSizeVisitor {
  template <typename T>
  int operator()(const T& element) {
    return element.PackedSize();
  }
};

struct SetMsgOffsetVisitor {
  const size_t offset;

  template <typename T>
  void operator()(T& element) {
    element.SetMsgOffset(offset);
  }
};

class Struct : public Element {
 public:
  using Element::Element;

  friend class BitfieldStruct;

  unsigned int PackedSize() const { return packed_size_; }

  AnyType *operator[](const std::string& name);
  void AddField(AnyType&& type);
  void AddField(const AnyType& type);
  void SetMsgOffset(size_t offset);
  void Unpack(const uint8_t *msg);

 private:
  unsigned int packed_size_ = 0;
  std::vector<Box<AnyType>> fields_;
};

class BitfieldStruct : public Struct {
 public:
  using Struct::Struct;

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
  using Element::Element;

  unsigned int PackedSize() const { return packed_size_; }

  void AddElem(AnyType&& type);
  void AddElem(const AnyType& type);
  void SetMsgOffset(size_t offset);
  void Unpack(const uint8_t *msg);

  Box<AnyType>& operator[](unsigned int i) { return elems_[i]; }

 private:
  unsigned int packed_size_ = 0;
  std::vector<Box<AnyType>> elems_;
};

inline AnyType *Struct::operator[](const std::string& name) {
  for (Box<AnyType>& field : fields_) {
    if (name == std::visit(GetInstNameVisitor{}, *field)) {
      return field.get();
    }
  }
  return nullptr;
}

inline void Struct::AddField(AnyType&& type) {
  packed_size_ += std::visit(PackedSizeVisitor{}, type);
  fields_.emplace_back(std::move(type));
  std::visit(SetMsgUidVisitor{GetMsgUid()}, *fields_.back());
}

inline void Struct::AddField(const AnyType& type) {
  packed_size_ += std::visit(PackedSizeVisitor{}, type);
  fields_.emplace_back(type);
  std::visit(SetMsgUidVisitor{GetMsgUid()}, *fields_.back());
}

inline void Struct::SetMsgOffset(size_t offset) {
  Element::SetMsgOffset(offset);

  for (auto& field : fields_) {
    std::visit([offset](auto&& f) { f.SetMsgOffset(offset); }, *field);
    offset += std::visit([](auto&& f) { return f.PackedSize(); }, *field);
  }
}

inline void Struct::Unpack(const uint8_t *msg) {
  for (auto& field : fields_) {
    std::visit([msg](auto&& f) { f.Unpack(msg); }, *field);
  }
}

inline void BitfieldStruct::AddBitfield(const std::string name, int bits) {
  int offset = 0;
  if (!fields_.empty()) {
    const int prev_offset = std::visit(GetBitOffsetVisitor{}, *fields_.back());
    const int prev_size = std::visit(GetBitSizeVisitor{}, *fields_.back());
    offset = prev_offset + prev_size;
  }

  const int packed_size_bits = offset + bits;
  if (packed_size_bits <= 8) {
    packed_size_ = 1;
  } else if (packed_size_bits <= 16) {
    packed_size_ = 2;
  } else if (packed_size_bits <= 32) {
    packed_size_ = 4;
  } else if (packed_size_bits <= 64) {
    packed_size_ = 8;
  } else {
    throw LogParseException("Bitfield struct too large.");
  }

  if (bits <= 8) {
    fields_.emplace_back(BitfieldPrimitive<uint8_t>{"uint8", name, packed_size_, offset, bits});
  } else if (bits <= 16) {
    fields_.emplace_back(BitfieldPrimitive<uint16_t>{"uint16", name, packed_size_, offset, bits});
  } else if (bits <= 32) {
    fields_.emplace_back(BitfieldPrimitive<uint32_t>{"uint32", name, packed_size_, offset, bits});
  } else if (bits <= 64) {
    fields_.emplace_back(BitfieldPrimitive<uint64_t>{"uint64", name, packed_size_, offset, bits});
  } else {
    throw LogParseException("Bitfield field too long.");
  }

  std::visit(SetMsgUidVisitor{GetMsgUid()}, *fields_.back());
}

inline void Array::AddElem(AnyType&& type) {
  packed_size_ += std::visit(PackedSizeVisitor{}, type);
  elems_.emplace_back(std::move(type));
  std::visit(SetMsgUidVisitor{GetMsgUid()}, *elems_.back());
}

inline void Array::AddElem(const AnyType& type) {
  packed_size_ += std::visit(PackedSizeVisitor{}, type);
  elems_.emplace_back(type);
  std::visit(SetMsgUidVisitor{GetMsgUid()}, *elems_.back());
}

inline void Array::SetMsgOffset(size_t offset) {
  Element::SetMsgOffset(offset);

  for (auto& elem : elems_) {
    std::visit([offset](auto&& e) { e.SetMsgOffset(offset); }, *elem);
    offset += std::visit([](auto&& e) { return e.PackedSize(); }, *elem);
  }
}

inline void Array::Unpack(const uint8_t *msg) {
  for (auto& elem : elems_) {
    std::visit([msg](auto&& e) { e.Unpack(msg); }, *elem);
  }
}

};  // namespace ss
