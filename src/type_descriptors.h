#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace ss {

class FieldDescriptor;

class TypeDescriptor {
 public:
  friend class DescriptorBuilder;

  enum class Type {
    kPrimitive,
    kEnum,
    kStruct,
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

  bool IsPrimitive() const { return type_ == Type::kPrimitive; }
  bool IsEnum() const { return type_ == Type::kEnum; }
  bool IsStruct() const { return type_ == Type::kStruct; }
  bool IsArray() const { return type_ == Type::kArray; }

  virtual PrimType prim_type() const { throw std::runtime_error("Type has no prim_type."); }

  virtual const std::vector<std::string>& enum_values() const {
    throw std::runtime_error("Type has no enum_values.");
  }

  virtual int array_size() const { throw std::runtime_error("Type has no array_size."); }
  virtual std::shared_ptr<TypeDescriptor> array_elem_type() const {
    throw std::runtime_error("Type has no array_elem_type().");
  }

  virtual const std::vector<std::unique_ptr<FieldDescriptor>>& struct_fields() const {
    throw std::runtime_error("Type has no struct_fields.");
  }

  virtual std::shared_ptr<TypeDescriptor> struct_get_field(std::string_view field_name) const {
    throw std::runtime_error("Type has no field lookup.");
  }

 protected:
  TypeDescriptor(std::string_view name, Type type) : name_{name}, type_{type} {}
  TypeDescriptor(const TypeDescriptor&) = delete;
  TypeDescriptor& operator=(const TypeDescriptor&) = delete;

  int packed_size_ = 0;

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
  std::shared_ptr<TypeDescriptor> type() const { return type_; }

 protected:
  FieldDescriptor(std::string_view name, std::shared_ptr<TypeDescriptor> type)
      : name_{name}, type_{type} {}
  FieldDescriptor(const FieldDescriptor&) = delete;
  FieldDescriptor& operator=(const FieldDescriptor&) = delete;

 private:
  std::string name_;
  std::shared_ptr<TypeDescriptor> type_;
};

class StructFieldDescriptor : public FieldDescriptor {
 protected:
  friend class StructDescriptor;

  StructFieldDescriptor(std::string_view name, std::shared_ptr<TypeDescriptor> type, int offset)
      : FieldDescriptor(name, type), offset_{offset} {
    (void)offset_;
  }
  StructFieldDescriptor(const StructFieldDescriptor&) = delete;
  StructFieldDescriptor& operator=(const StructFieldDescriptor&) = delete;

 private:
  int offset_;
};

class BitfieldFieldDescriptor : public FieldDescriptor {
 protected:
  friend class BitfieldDescriptor;

  BitfieldFieldDescriptor(std::string_view name, std::shared_ptr<TypeDescriptor> type,
                          int bit_offset, int bit_size)
      : FieldDescriptor(name, type), bit_offset_{bit_offset}, bit_size_{bit_size} {
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

  const std::vector<std::unique_ptr<FieldDescriptor>>& struct_fields() const override {
    return fields_;
  }

  std::shared_ptr<TypeDescriptor> struct_get_field(std::string_view field_name) const override {
    for (auto& field : fields_) {
      if (field->name() == field_name) return field->type();
    }
    return nullptr;
  }

 protected:
  StructDescriptor(std::string_view name) : TypeDescriptor(name, Type::kStruct) {}
  StructDescriptor(const StructDescriptor&) = delete;
  StructDescriptor& operator=(const StructDescriptor&) = delete;

  void AddField(const std::string& name, const std::shared_ptr<TypeDescriptor>& field) {
    fields_.emplace_back(new StructFieldDescriptor(name, field, packed_size_));
    packed_size_ += field->packed_size();
  }

 private:
  std::vector<std::unique_ptr<FieldDescriptor>> fields_;
};

class BitfieldDescriptor : public TypeDescriptor {
 public:
  friend class DescriptorBuilder;

  const std::vector<std::unique_ptr<FieldDescriptor>>& struct_fields() const override {
    return fields_;
  }

 protected:
  BitfieldDescriptor(std::string_view name) : TypeDescriptor(name, Type::kStruct) {}
  BitfieldDescriptor(const BitfieldDescriptor&) = delete;
  BitfieldDescriptor& operator=(const BitfieldDescriptor&) = delete;

  void AddField(const std::string& name, const std::shared_ptr<TypeDescriptor>& field,
                int bit_size) {
    fields_.emplace_back(new BitfieldFieldDescriptor(name, field, cur_bit_offset_, bit_size));
    cur_bit_offset_ += bit_size;
    SetPackedSize();
  }

 private:
  void SetPackedSize() {
    if (cur_bit_offset_ <= 8) {
      packed_size_ = 1;
    } else if (cur_bit_offset_ <= 16) {
      packed_size_ = 2;
    } else if (cur_bit_offset_ <= 32) {
      packed_size_ = 4;
    } else if (cur_bit_offset_ <= 64) {
      packed_size_ = 8;
    } else {
      throw std::runtime_error("Bitfield too big.");
    }
  }

  std::vector<std::unique_ptr<FieldDescriptor>> fields_;
  int cur_bit_offset_ = 0;
};

class ArrayDescriptor : public TypeDescriptor {
 public:
  friend class DescriptorBuilder;

  int array_size() const override { return size_; }
  std::shared_ptr<TypeDescriptor> array_elem_type() const override { return elem_; }

 protected:
  ArrayDescriptor(std::shared_ptr<TypeDescriptor> elem, int size)
      : TypeDescriptor(ArrayName(*elem, size), Type::kArray), elem_{elem}, size_{size} {
    packed_size_ = elem->packed_size() * size;
  }

  ArrayDescriptor(const ArrayDescriptor&) = delete;
  ArrayDescriptor& operator=(const ArrayDescriptor&) = delete;

 private:
  static std::string ArrayName(const TypeDescriptor& elem, int size) {
    return elem.name() + "[" + std::to_string(size) + "]";
  }

  std::shared_ptr<TypeDescriptor> elem_;
  int size_;
};

class DescriptorBuilder {
 public:
  using TypeMap = std::unordered_map<std::string, std::shared_ptr<TypeDescriptor>>;

  static TypeMap FromFile(const std::string& filename);
  static TypeMap FromString(const std::string& str);

 private:
  static TypeMap FromNode(const YAML::Node& root_node);

  static std::shared_ptr<TypeDescriptor> ParseStruct(std::string_view name, const YAML::Node& node,
                                                     TypeMap& type_map, bool is_msg);
  static std::shared_ptr<TypeDescriptor> ParseArray(const YAML::Node& node, TypeMap& type_map);
  static std::shared_ptr<TypeDescriptor> ParseEnum(std::string_view name, const YAML::Node& node);
  static std::shared_ptr<TypeDescriptor> ParseBitfield(std::string_view name,
                                                       const YAML::Node& node,
                                                       const TypeMap& type_map);
};

};  // namespace ss
