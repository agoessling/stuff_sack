#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

#include "src/type_descriptors.h"

namespace ss {

class DynamicArray;
class DynamicStruct;

using AnyField =
    std::variant<std::unique_ptr<uint8_t>, std::unique_ptr<uint16_t>, std::unique_ptr<uint32_t>,
                 std::unique_ptr<uint64_t>, std::unique_ptr<int8_t>, std::unique_ptr<int16_t>,
                 std::unique_ptr<int32_t>, std::unique_ptr<int64_t>, std::unique_ptr<bool>,
                 std::unique_ptr<float>, std::unique_ptr<double>, std::unique_ptr<DynamicArray>,
                 std::unique_ptr<DynamicStruct>>;

static inline AnyField MakeAnyField(const TypeDescriptor *field_type_descriptor) {
  using Type = TypeDescriptor::Type;
  using PrimType = TypeDescriptor::PrimType;

  switch (field_type_descriptor->type()) {
    case Type::kPrimitive:
    case Type::kEnum:
      switch (field_type_descriptor->prim_type()) {
        case PrimType::kUint8:
          return std::make_unique<uint8_t>();
        case PrimType::kUint16:
          return std::make_unique<uint16_t>();
        case PrimType::kUint32:
          return std::make_unique<uint32_t>();
        case PrimType::kUint64:
          return std::make_unique<uint64_t>();
        case PrimType::kInt8:
          return std::make_unique<int8_t>();
        case PrimType::kInt16:
          return std::make_unique<int16_t>();
        case PrimType::kInt32:
          return std::make_unique<int32_t>();
        case PrimType::kInt64:
          return std::make_unique<int64_t>();
        case PrimType::kBool:
          return std::make_unique<bool>();
        case PrimType::kFloat:
          return std::make_unique<float>();
        case PrimType::kDouble:
          return std::make_unique<double>();
      }
    case Type::kArray:
      return std::make_unique<DynamicArray>(*field_type_descriptor);
    case Type::kStruct:
      return std::make_unique<DynamicStruct>(*field_type_descriptor);
  }

  return {};
}

class DynamicStruct {
 public:
  DynamicStruct(const TypeDescriptor& descriptor) : descriptor_{descriptor} {
    for (const std::unique_ptr<const FieldDescriptor>& field : descriptor.struct_fields()) {
      fields_.emplace(field.get(), MakeAnyField(field->type()));
    }
  }

  template <typename T>
  T& Get(const FieldDescriptor& field_descriptor) {
    return *std::get<std::unique_ptr<T>>(fields_.at(&field_descriptor)).get();
  }

  template <typename T>
  T& Get(std::string_view field_name) {
    return Get<T>(*descriptor_[field_name]);
  }

  template <typename T>
  T *GetIf(const FieldDescriptor& field_descriptor) {
    const auto& field_it = fields_.find(&field_descriptor);
    if (field_it == fields_.end()) return nullptr;
    return std::get<std::unique_ptr<T>>(field_it->second).get();
  }

  template <typename T>
  T *GetIf(std::string_view field_name) {
    const FieldDescriptor *field = descriptor_[field_name];
    if (!field) return nullptr;
    return GetIf<T>(*field);
  }

  template <typename T>
  T Convert(const FieldDescriptor& field_descriptor) {
    AnyField& field = fields_.at(&field_descriptor);

    return std::visit(
        [](auto&& field) {
          using U = typename std::decay_t<decltype(field)>::element_type;

          if constexpr (std::is_same_v<U, DynamicStruct> || std::is_same_v<U, DynamicArray>) {
            throw std::bad_variant_access();
            return T{};
          } else {
            return static_cast<T>(*field.get());
          }
        },
        field);
  }

  template <typename T>
  T Convert(std::string_view field_name) {
    return Convert<T>(*descriptor_[field_name]);
  }

  template <typename T>
  std::optional<T> ConvertIf(const FieldDescriptor& field_descriptor) {
    const auto& field_it = fields_.find(&field_descriptor);
    if (field_it == fields_.end()) return std::nullopt;

    return std::visit(
        [](auto&& field) {
          using U = typename std::decay_t<decltype(field)>::element_type;

          if constexpr (std::is_same_v<U, DynamicStruct> || std::is_same_v<U, DynamicArray>) {
            throw std::bad_variant_access();
            return T{};
          } else {
            return static_cast<T>(*field.get());
          }
        },
        field_it->second);
  }

  template <typename T>
  std::optional<T> ConvertIf(std::string_view field_name) {
    const FieldDescriptor *field = descriptor_[field_name];
    if (!field) return std::nullopt;
    return ConvertIf<T>(*field);
  }

  const TypeDescriptor& descriptor() const { return descriptor_; }

 private:
  std::unordered_map<const FieldDescriptor *, AnyField> fields_;
  const TypeDescriptor& descriptor_;
};

class DynamicArray {
 public:
  DynamicArray(const TypeDescriptor& descriptor)
      : descriptor_{descriptor}, elems_(descriptor.array_size()) {
    for (size_t i = 0; i < elems_.size(); ++i) {
      elems_[i] = MakeAnyField(descriptor.array_elem_type());
    }
  }

  template <typename T>
  T& Get(size_t i) {
    return *std::get<std::unique_ptr<T>>(elems_[i]).get();
  }

  template <typename T>
  T Convert(size_t i) {
    AnyField& field = elems_[i];

    return std::visit(
        [](auto&& field) {
          using U = typename std::decay_t<decltype(field)>::element_type;

          if constexpr (std::is_same_v<U, DynamicStruct> || std::is_same_v<U, DynamicArray>) {
            throw std::bad_variant_access();
          }

          return static_cast<T>(*field.get());
        },
        field);
  }

  size_t size() const { return elems_.size(); };
  const TypeDescriptor& descriptor() const { return descriptor_; }

 private:
  const TypeDescriptor& descriptor_;
  std::vector<AnyField> elems_;
};

}  // namespace ss
