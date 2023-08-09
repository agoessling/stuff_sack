#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "uid_hash.h"

namespace ss {

class FieldDescriptor;

class TypeDescriptor {
 public:
  friend class DescriptorBuilder;

  using FieldList = std::vector<std::unique_ptr<const FieldDescriptor>>;

  enum class Type {
    kPrimitive,
    kEnum,
    kStruct,
    kBitfield,
    kArray,
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

  Type type() const { return type_; }
  int packed_size() const { return packed_size_; }
  const std::string& name() const { return name_; }
  uint32_t uid() const { return uid_; }

  bool IsPrimitive() const { return type_ == Type::kPrimitive; }
  bool IsEnum() const { return type_ == Type::kEnum; }
  bool IsStruct() const { return type_ == Type::kStruct; }
  bool IsBitfield() const { return type_ == Type::kBitfield; }
  bool IsArray() const { return type_ == Type::kArray; }

  virtual PrimType prim_type() const { throw std::runtime_error("Type has no prim_type."); }

  virtual const std::vector<std::string>& enum_values() const {
    throw std::runtime_error("Type has no enum_values.");
  }

  virtual int array_size() const { throw std::runtime_error("Type has no array_size."); }
  virtual const TypeDescriptor *array_elem_type() const {
    throw std::runtime_error("Type has no array_elem_type().");
  }

  virtual const FieldList& struct_fields() const {
    throw std::runtime_error("Type has no struct_fields.");
  }

  virtual const FieldDescriptor *operator[](std::string_view field_name) const {
    throw std::runtime_error("Type has no field lookup.");
  }

 protected:
  TypeDescriptor(std::string_view name, Type type) : name_{name}, type_{type} {}
  TypeDescriptor(const TypeDescriptor&) = delete;
  TypeDescriptor& operator=(const TypeDescriptor&) = delete;

  int packed_size_ = 0;
  uint32_t uid_ = 0;

 private:
  std::string name_;
  Type type_;
};

class PrimitiveDescriptor : public TypeDescriptor {
 public:
  friend class DescriptorBuilder;

  PrimType prim_type() const override { return prim_type_; }

 protected:
  PrimitiveDescriptor(std::string_view name, PrimType prim_type)
      : TypeDescriptor(name, Type::kPrimitive) {
    SetPrimType(prim_type);
  };

  // Used by subclasses (e.g. Enum).
  PrimitiveDescriptor(std::string_view name, Type type, PrimType prim_type)
      : TypeDescriptor(name, type) {
    SetPrimType(prim_type);
  };

  PrimitiveDescriptor(const PrimitiveDescriptor&) = delete;
  PrimitiveDescriptor& operator=(const PrimitiveDescriptor&) = delete;

  void SetPrimType(PrimType prim_type) {
    prim_type_ = prim_type;
    SetPackedSize();
  }

 private:
  void SetUid() { uid_ = PrimitiveHash(name().c_str(), packed_size_); }

  void SetPackedSize() {
    switch (prim_type_) {
      case PrimType::kUint8:
      case PrimType::kInt8:
      case PrimType::kBool:
        packed_size_ = 1;
        break;
      case PrimType::kUint16:
      case PrimType::kInt16:
        packed_size_ = 2;
        break;
      case PrimType::kUint32:
      case PrimType::kInt32:
      case PrimType::kFloat:
        packed_size_ = 4;
        break;
      case PrimType::kUint64:
      case PrimType::kInt64:
      case PrimType::kDouble:
        packed_size_ = 8;
        break;
    }
    SetUid();
  }

  PrimType prim_type_;
};

class EnumDescriptor : public PrimitiveDescriptor {
 public:
  friend class DescriptorBuilder;

  const std::vector<std::string>& enum_values() const override { return values_; }

 protected:
  EnumDescriptor(std::string_view name) : PrimitiveDescriptor(name, Type::kEnum, PrimType::kInt8) {}
  EnumDescriptor(const EnumDescriptor&) = delete;
  EnumDescriptor& operator=(const EnumDescriptor&) = delete;

  void AddValue(std::string_view value) {
    values_.emplace_back(value);

    if (values_.size() <= (1ULL << 7) - 1) {
      SetPrimType(PrimType::kInt8);
    } else if (values_.size() <= (1ULL << 15) - 1) {
      SetPrimType(PrimType::kInt16);
    } else if (values_.size() <= (1ULL << 31) - 1) {
      SetPrimType(PrimType::kInt32);
    } else if (values_.size() <= (1ULL << 63) - 1) {
      SetPrimType(PrimType::kInt64);
    } else {
      throw std::runtime_error("Too many enum values.");
    }
  }

 private:
  std::vector<std::string> values_;
};

class FieldDescriptor {
 public:
  const std::string& name() const { return name_; }
  const TypeDescriptor *type() const { return type_; }
  uint32_t uid() const { return uid_; }

  virtual int offset() const { throw std::runtime_error("Field does not have offset."); }
  virtual int bit_offset() const { throw std::runtime_error("Field does not have bit_offset."); }
  virtual int bit_size() const { throw std::runtime_error("Field does not have bit_size."); }

 protected:
  FieldDescriptor(const std::string& name, const TypeDescriptor *type, uint32_t uid)
      : name_{name}, type_{type}, uid_{uid} {}
  FieldDescriptor(const FieldDescriptor&) = delete;
  FieldDescriptor& operator=(const FieldDescriptor&) = delete;

 private:
  std::string name_;
  const TypeDescriptor *type_;
  uint32_t uid_;
};

class StructFieldDescriptor : public FieldDescriptor {
 public:
  int offset() const override { return offset_; }

 protected:
  friend class StructDescriptor;

  StructFieldDescriptor(const std::string& name, const TypeDescriptor *type, int offset)
      : FieldDescriptor(name, type, StructFieldHash(name.c_str(), type->uid())), offset_{offset} {}
  StructFieldDescriptor(const StructFieldDescriptor&) = delete;
  StructFieldDescriptor& operator=(const StructFieldDescriptor&) = delete;

 private:
  int offset_;
};

class BitfieldFieldDescriptor : public FieldDescriptor {
 public:
  int bit_offset() const override { return bit_offset_; }
  int bit_size() const override { return bit_size_; }

 protected:
  friend class BitfieldDescriptor;

  BitfieldFieldDescriptor(const std::string& name, const TypeDescriptor *type, int bit_offset,
                          int bit_size)
      : FieldDescriptor(name, type, BitfieldFieldHash(name.c_str(), bit_size)),
        bit_offset_{bit_offset},
        bit_size_{bit_size} {
    (void)bit_offset_;
    (void)bit_size_;
  }
  BitfieldFieldDescriptor(const BitfieldFieldDescriptor&) = delete;
  BitfieldFieldDescriptor& operator=(const BitfieldFieldDescriptor&) = delete;

 private:
  int bit_offset_;
  int bit_size_;
};

class StructDescriptor : public TypeDescriptor {
 public:
  friend class DescriptorBuilder;

  const FieldList& struct_fields() const override { return fields_; }

  const FieldDescriptor *operator[](std::string_view field_name) const override {
    for (auto& field : fields_) {
      if (field->name() == field_name) return field.get();
    }
    return nullptr;
  }

 protected:
  StructDescriptor(std::string_view name) : TypeDescriptor(name, Type::kStruct) {}
  StructDescriptor(const StructDescriptor&) = delete;
  StructDescriptor& operator=(const StructDescriptor&) = delete;

  void AddField(const std::string& name, const TypeDescriptor *field) {
    fields_.emplace_back(new StructFieldDescriptor(name, field, packed_size_));
    packed_size_ += field->packed_size();
    SetUid();
  }

 private:
  void SetUid() {
    std::vector<uint32_t> field_uids(fields_.size());
    for (size_t i = 0; i < fields_.size(); ++i) {
      field_uids[i] = fields_[i]->uid();
    }
    uid_ = StructHash(name().c_str(), field_uids.data(), field_uids.size());
  }

  FieldList fields_;
};

class BitfieldDescriptor : public TypeDescriptor {
 public:
  friend class DescriptorBuilder;

  PrimType prim_type() const override { return prim_type_; }
  const FieldList& struct_fields() const override { return fields_; }

  const FieldDescriptor *operator[](std::string_view field_name) const override {
    for (auto& field : fields_) {
      if (field->name() == field_name) return field.get();
    }
    return nullptr;
  }

 protected:
  BitfieldDescriptor(std::string_view name) : TypeDescriptor(name, Type::kBitfield) {}
  BitfieldDescriptor(const BitfieldDescriptor&) = delete;
  BitfieldDescriptor& operator=(const BitfieldDescriptor&) = delete;

  void AddField(const std::string& name, const TypeDescriptor *field, int bit_size) {
    fields_.emplace_back(new BitfieldFieldDescriptor(name, field, cur_bit_offset_, bit_size));
    cur_bit_offset_ += bit_size;
    SetBitfieldSize();
    SetUid();
  }

 private:
  void SetUid() {
    std::vector<uint32_t> field_uids(fields_.size());
    for (size_t i = 0; i < fields_.size(); ++i) {
      field_uids[i] = fields_[i]->uid();
    }
    uid_ = BitfieldHash(name().c_str(), field_uids.data(), field_uids.size());
  }

  void SetBitfieldSize() {
    if (cur_bit_offset_ <= 8) {
      prim_type_ = PrimType::kUint8;
      packed_size_ = 1;
    } else if (cur_bit_offset_ <= 16) {
      prim_type_ = PrimType::kUint16;
      packed_size_ = 2;
    } else if (cur_bit_offset_ <= 32) {
      prim_type_ = PrimType::kUint32;
      packed_size_ = 4;
    } else if (cur_bit_offset_ <= 64) {
      prim_type_ = PrimType::kUint64;
      packed_size_ = 8;
    } else {
      throw std::runtime_error("Bitfield too big.");
    }
  }

  FieldList fields_;
  PrimType prim_type_ = PrimType::kUint8;
  int cur_bit_offset_ = 0;
};

class ArrayDescriptor : public TypeDescriptor {
 public:
  friend class DescriptorBuilder;

  int array_size() const override { return size_; }
  const TypeDescriptor *array_elem_type() const override { return elem_; }

 protected:
  ArrayDescriptor(const TypeDescriptor *elem, int size)
      : TypeDescriptor(ArrayName(*elem, size), Type::kArray), elem_{elem}, size_{size} {
    packed_size_ = elem->packed_size() * size;
    uid_ = ArrayHash(elem->uid(), size);
  }

  ArrayDescriptor(const ArrayDescriptor&) = delete;
  ArrayDescriptor& operator=(const ArrayDescriptor&) = delete;

 private:
  static std::string ArrayName(const TypeDescriptor& elem, int size) {
    return elem.name() + "[" + std::to_string(size) + "]";
  }

  const TypeDescriptor *elem_;
  int size_;
};

class DescriptorBuilder {
 public:
  using TypeMap = std::unordered_map<std::string, std::unique_ptr<TypeDescriptor>>;

  static DescriptorBuilder FromFile(const std::string& filename);
  static DescriptorBuilder FromString(const std::string& str);

  DescriptorBuilder(const YAML::Node& root_node);

  const TypeDescriptor *operator[](const std::string& name) const {
    return type_map_.at(name).get();
  }

  const TypeMap& types() const { return type_map_; }

 private:
  void ParseStruct(std::string_view name, const YAML::Node& node, bool is_msg);
  TypeDescriptor *ParseArray(const YAML::Node& node);
  void ParseEnum(std::string_view name, const YAML::Node& node);
  void ParseBitfield(std::string_view name, const YAML::Node& node);

 private:
  TypeMap type_map_;
};

};  // namespace ss
