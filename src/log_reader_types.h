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

class TypeBox;

class Type {
 public:
  friend class TypeBox;

  Type() = default;
  Type(std::string name, std::string type_name) : name{name}, type_name{type_name} {}
  virtual ~Type() = default;

  virtual size_t PackedSize() const = 0;
  virtual void Unpack(const uint8_t *msg) = 0;

  virtual TypeBox& operator[](const std::string& field_name) {
    throw LogReaderException("String key not supported.");
  }

  virtual TypeBox& operator[](size_t i) { throw LogReaderException("Integer key not supported."); }

  virtual void SetMsgInfo(uint32_t uid, size_t offset) {
    msg_uid = uid;
    msg_offset_ = offset;
  }

  void SetInstanceName(std::string new_name) {
    type_name = std::move(name);
    name = new_name;
  }

  std::string name;
  std::string type_name;
  uint32_t msg_uid;

 protected:
  size_t msg_offset_;

 private:
  virtual Type *Clone() const = 0;
};

class TypeBox {
 public:
  TypeBox() = default;
  TypeBox(const Type& type) : ptr_(type.Clone()) {}

  TypeBox(const TypeBox& other) : TypeBox(*other.ptr_) {}
  TypeBox& operator=(const TypeBox& other) {
    ptr_.reset(other.ptr_->Clone());
    return *this;
  }

  ~TypeBox() = default;

  Type& operator*() { return *ptr_; }
  const Type& operator*() const { return *ptr_; }

  Type *operator->() { return ptr_.get(); }
  const Type *operator->() const { return ptr_.get(); }

  Type *get() { return ptr_.get(); }
  const Type *get() const { return ptr_.get(); }

  template <typename T>
  TypeBox& operator[](T key) {
    return (*ptr_)[key];
  }

 private:
  std::unique_ptr<Type> ptr_;
};

template <typename T>
class Primitive : public Type {
 public:
  Primitive() = default;
  Primitive(std::string name, std::string type_name) : Type(name, type_name) {}

  size_t PackedSize() const override { return sizeof(T); }

  void Unpack(const uint8_t *msg) override {
    data.push_back(UnpackPrimitive<T>(msg + msg_offset_));
  }

  std::vector<T> data;

 private:
  Primitive *Clone() const override { return new Primitive(*this); }
};

template <typename T, typename U>
class BitfieldPrimitive : public Primitive<T> {
 public:
  BitfieldPrimitive() = default;
  BitfieldPrimitive(std::string name, std::string type_name, int bit_offset, int bit_size)
      : Primitive<T>(name, type_name), bit_offset{bit_offset}, bit_size{bit_size} {}

  void Unpack(const uint8_t *msg) override {
    const U raw_data = UnpackPrimitive<U>(msg + this->msg_offset_);
    const U mask = (1 << bit_size) - 1;
    this->data.push_back((raw_data >> bit_offset) & mask);
  }

  int bit_offset;
  int bit_size;

 private:
  BitfieldPrimitive *Clone() const override { return new BitfieldPrimitive(*this); }
};

template <typename T>
class Enum : public Primitive<T> {
 public:
  Enum() = default;
  Enum(std::string name, std::string type_name) : Primitive<T>(name, type_name) {}

  void SetValues(std::vector<std::string> values) { values_ = values; }

 private:
  Enum *Clone() const override { return new Enum(*this); }

  std::vector<std::string> values_;
};

class Struct : public Type {
 public:
  Struct() = default;
  Struct(std::string name, std::string type_name) : Type(name, type_name) {}

  size_t PackedSize() const override { return packed_size_; }

  void Unpack(const uint8_t *msg) override {
    for (auto& field : fields_) {
      field->Unpack(msg);
    }
  }

  void SetMsgInfo(uint32_t uid, size_t offset) override {
    msg_uid = uid;
    msg_offset_ = offset;
    for (auto& field : fields_) {
      field->SetMsgInfo(uid, offset);
      offset += field->PackedSize();
    }
  }

  TypeBox& operator[](const std::string& name) override {
    for (TypeBox& box : fields_) {
      if (box->name == name) {
        return box;
      }
    }

    throw LogReaderException("Key not found: " + name);
  }

  void AddField(const Type& type) {
    packed_size_ += type.PackedSize();
    fields_.emplace_back(type);
  }

 protected:
  std::vector<TypeBox> fields_;
  size_t packed_size_ = 0;

 private:
  Struct *Clone() const override { return new Struct(*this); }
};

class BitfieldStruct : public Struct {
 public:
  BitfieldStruct() = default;
  BitfieldStruct(std::string name, std::string type_name) : Struct(name, type_name) {}

  void SetMsgInfo(uint32_t uid, size_t offset) override {
    msg_uid = uid;
    msg_offset_ = offset;
    for (auto& field : fields_) {
      field->SetMsgInfo(uid, offset);
    }
  }

  void AddBitfield(const std::string name, int bits) {
    const int packed_size_bits = cur_offset_ + bits;
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
      if (packed_size_ == 1) {
        fields_.emplace_back(BitfieldPrimitive<uint8_t, uint8_t>{name, "uint8", cur_offset_, bits});
      } else if (packed_size_ == 2) {
        fields_.emplace_back(
            BitfieldPrimitive<uint8_t, uint16_t>{name, "uint8", cur_offset_, bits});
      } else if (packed_size_ == 4) {
        fields_.emplace_back(
            BitfieldPrimitive<uint8_t, uint32_t>{name, "uint8", cur_offset_, bits});
      } else if (packed_size_ == 8) {
        fields_.emplace_back(
            BitfieldPrimitive<uint8_t, uint64_t>{name, "uint8", cur_offset_, bits});
      }
    } else if (bits <= 16) {
      if (packed_size_ == 2) {
        fields_.emplace_back(
            BitfieldPrimitive<uint16_t, uint16_t>{name, "uint8", cur_offset_, bits});
      } else if (packed_size_ == 4) {
        fields_.emplace_back(
            BitfieldPrimitive<uint16_t, uint32_t>{name, "uint8", cur_offset_, bits});
      } else if (packed_size_ == 8) {
        fields_.emplace_back(
            BitfieldPrimitive<uint16_t, uint64_t>{name, "uint8", cur_offset_, bits});
      }
    } else if (bits <= 32) {
      if (packed_size_ == 4) {
        fields_.emplace_back(
            BitfieldPrimitive<uint32_t, uint32_t>{name, "uint8", cur_offset_, bits});
      } else if (packed_size_ == 8) {
        fields_.emplace_back(
            BitfieldPrimitive<uint32_t, uint64_t>{name, "uint8", cur_offset_, bits});
      }
    } else if (bits <= 64) {
      fields_.emplace_back(
          BitfieldPrimitive<uint64_t, uint64_t>{name, "uint64", cur_offset_, bits});
    } else {
      throw LogParseException("Bitfield field too long.");
    }

    cur_offset_ += bits;
  }

 private:
  BitfieldStruct *Clone() const override { return new BitfieldStruct(*this); }

  int cur_offset_ = 0;
};

class Array : public Type {
 public:
  Array() = default;
  Array(std::string name) : Type(name, "") {}

  size_t PackedSize() const override { return packed_size_; }

  void Unpack(const uint8_t *msg) override {
    for (auto& elem : elems_) {
      elem->Unpack(msg);
    }
  }

  void SetMsgInfo(uint32_t uid, size_t offset) override {
    msg_uid = uid;
    msg_offset_ = offset;
    for (auto& elem : elems_) {
      elem->SetMsgInfo(uid, offset);
      offset += elem->PackedSize();
    }
  }

  TypeBox& operator[](size_t i) override {
    if (i >= elems_.size()) {
      throw LogReaderException("Index out of range: " + std::to_string(i));
    }

    return elems_[i];
  }

  void AddElem(const Type& type) {
    if (type_name.empty()) {
      type_name = type.type_name;
    }
    packed_size_ += type.PackedSize();
    elems_.emplace_back(type);
  }

 private:
  Array *Clone() const override { return new Array(*this); }

  size_t packed_size_ = 0;
  std::vector<TypeBox> elems_;
};

};  // namespace ss
