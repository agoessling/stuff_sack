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
    uint16_t raw_value = (static_cast<uint16_t>(data[0]) << static_cast<uint16_t>(8)) |
                         (static_cast<uint16_t>(data[1]) << static_cast<uint16_t>(0));
    memcpy(&value, &raw_value, sizeof(value));
  } else if constexpr (sizeof(value) == 4) {
    uint32_t raw_value = (static_cast<uint32_t>(data[0]) << static_cast<uint32_t>(24)) |
                         (static_cast<uint32_t>(data[1]) << static_cast<uint32_t>(16)) |
                         (static_cast<uint32_t>(data[2]) << static_cast<uint32_t>(8)) |
                         (static_cast<uint32_t>(data[3]) << static_cast<uint32_t>(0));
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

enum class ElemType {
  kPrimitive,
  kStruct,
  kEnum,
  kArray,
};

class TypeBox;
class Struct;
class Array;

class Type {
 public:
  friend class LogReader;
  friend class Array;
  friend class BitfieldStruct;
  friend class Struct;
  friend class TypeBox;

  Type() = default;
  Type(std::string type_name) : type_name_{std::move(type_name)} {}
  Type(std::string inst_name, std::string type_name)
      : inst_name_{std::move(inst_name)}, type_name_{std::move(type_name)} {}

  virtual ~Type() = default;

  virtual size_t PackedSize() const = 0;
  virtual enum ElemType ElemType() const = 0;

  virtual TypeBox& operator[](std::string_view field_name) {
    throw LogReaderException("String key not supported.");
  }

  virtual TypeBox& operator[](size_t i) { throw LogReaderException("Integer key not supported."); }

  const std::string& inst_name() { return inst_name_; }
  const std::string& type_name() { return type_name_; }

 protected:
  virtual void Unpack(const uint8_t *msg) = 0;

  virtual void SetMsgInfo(uint32_t uid, size_t offset) {
    msg_uid_ = uid;
    msg_offset_ = offset;
  }

  std::string inst_name_;
  std::string type_name_;
  uint32_t msg_uid_;
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
  friend class LogReader;
  using Type::Type;

  size_t PackedSize() const override { return sizeof(T); }
  enum ElemType ElemType() const override { return ElemType::kPrimitive; }

 protected:
  void Unpack(const uint8_t *msg) override {
    data_.push_back(UnpackPrimitive<T>(msg + msg_offset_));
  }

  std::vector<T> data_;

 private:
  Primitive *Clone() const override { return new Primitive(*this); }
};

template <typename T, typename U>
class BitfieldPrimitive : public Primitive<T> {
 public:
  friend class LogReader;
  using Primitive<T>::Primitive;

  BitfieldPrimitive(std::string inst_name, std::string type_name, int bit_offset, int bit_size)
      : Primitive<T>(std::move(inst_name), std::move(type_name)),
        bit_offset_{bit_offset},
        bit_size_{bit_size} {}

 protected:
  void Unpack(const uint8_t *msg) override {
    const U raw_data = UnpackPrimitive<U>(msg + this->msg_offset_);
    const U mask = (1 << bit_size_) - 1;
    this->data_.push_back((raw_data >> bit_offset_) & mask);
  }

  int bit_offset_;
  int bit_size_;

 private:
  BitfieldPrimitive *Clone() const override { return new BitfieldPrimitive(*this); }
};

template <typename T>
class Enum : public Primitive<T> {
 public:
  friend class LogReader;
  using Primitive<T>::Primitive;

  Enum(std::string type_name, std::vector<std::string> values)
      : Primitive<T>(std::move(type_name)), values_{std::move(values)} {}

  enum ElemType ElemType() const override { return ElemType::kEnum; }

 protected:
  void SetValues(std::vector<std::string> values) { values_ = std::move(values); }

  std::vector<std::string> values_;

 private:
  Enum *Clone() const override { return new Enum(*this); }
};

class Struct : public Type {
 public:
  friend class LogReader;
  using Type::Type;

  size_t PackedSize() const override { return packed_size_; }
  enum ElemType ElemType() const override { return ElemType::kStruct; }

 protected:
  void Unpack(const uint8_t *msg) override {
    for (TypeBox& field : fields_) {
      field->Unpack(msg);
    }
  }

  void SetMsgInfo(uint32_t uid, size_t offset) override {
    Type::SetMsgInfo(uid, offset);

    for (TypeBox& field : fields_) {
      field->SetMsgInfo(uid, offset);
      offset += field->PackedSize();
    }
  }

  TypeBox& operator[](std::string_view name) override {
    for (TypeBox& box : fields_) {
      if (box->inst_name_ == name) {
        return box;
      }
    }

    throw LogReaderException("Key not found: " + inst_name_);
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
  friend class LogReader;
  using Struct::Struct;

 protected:
  void SetMsgInfo(uint32_t uid, size_t offset) override {
    Type::SetMsgInfo(uid, offset);

    for (TypeBox& field : fields_) {
      field->SetMsgInfo(uid, offset);
    }
  }

  void AddBitfield(std::string name, int bits) {
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
        fields_.emplace_back(
            BitfieldPrimitive<uint8_t, uint8_t>{std::move(name), "uint8", cur_offset_, bits});
      } else if (packed_size_ == 2) {
        fields_.emplace_back(
            BitfieldPrimitive<uint8_t, uint16_t>{std::move(name), "uint8", cur_offset_, bits});
      } else if (packed_size_ == 4) {
        fields_.emplace_back(
            BitfieldPrimitive<uint8_t, uint32_t>{std::move(name), "uint8", cur_offset_, bits});
      } else if (packed_size_ == 8) {
        fields_.emplace_back(
            BitfieldPrimitive<uint8_t, uint64_t>{std::move(name), "uint8", cur_offset_, bits});
      }
    } else if (bits <= 16) {
      if (packed_size_ == 2) {
        fields_.emplace_back(
            BitfieldPrimitive<uint16_t, uint16_t>{std::move(name), "uint16", cur_offset_, bits});
      } else if (packed_size_ == 4) {
        fields_.emplace_back(
            BitfieldPrimitive<uint16_t, uint32_t>{std::move(name), "uint16", cur_offset_, bits});
      } else if (packed_size_ == 8) {
        fields_.emplace_back(
            BitfieldPrimitive<uint16_t, uint64_t>{std::move(name), "uint16", cur_offset_, bits});
      }
    } else if (bits <= 32) {
      if (packed_size_ == 4) {
        fields_.emplace_back(
            BitfieldPrimitive<uint32_t, uint32_t>{std::move(name), "uint32", cur_offset_, bits});
      } else if (packed_size_ == 8) {
        fields_.emplace_back(
            BitfieldPrimitive<uint32_t, uint64_t>{std::move(name), "uint32", cur_offset_, bits});
      }
    } else if (bits <= 64) {
      fields_.emplace_back(
          BitfieldPrimitive<uint64_t, uint64_t>{std::move(name), "uint64", cur_offset_, bits});
    } else {
      throw LogParseException("Bitfield field too long.");
    }

    cur_offset_ += bits;
  }

  int cur_offset_ = 0;

 private:
  BitfieldStruct *Clone() const override { return new BitfieldStruct(*this); }
};

class Array : public Type {
 public:
  friend class LogReader;
  using Type::Type;

  Array(std::string name) : Type(std::move(name), "") {}

  size_t PackedSize() const override { return packed_size_; }
  enum ElemType ElemType() const override { return ElemType::kArray; }

 protected:
  void Unpack(const uint8_t *msg) override {
    for (TypeBox& elem : elems_) {
      elem->Unpack(msg);
    }
  }

  void SetMsgInfo(uint32_t uid, size_t offset) override {
    Type::SetMsgInfo(uid, offset);

    for (TypeBox& elem : elems_) {
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
    if (type_name_.empty()) {
      type_name_ = type.type_name_;
    }
    packed_size_ += type.PackedSize();
    elems_.emplace_back(type);
  }

  size_t packed_size_ = 0;
  std::vector<TypeBox> elems_;

 private:
  Array *Clone() const override { return new Array(*this); }
};

};  // namespace ss
