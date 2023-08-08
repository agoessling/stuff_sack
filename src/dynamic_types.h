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

namespace impl {

template <typename T>
class Box {
 public:
  template <typename ...Args>
  static Box<T> Make(Args&&... args) {
    return Box<T>(std::make_unique<T>(std::forward<Args>(args)...));
  }

  Box() = delete;

  Box(const T& val) : ptr_(std::make_unique<T>(val)) {}
  Box(T&& val) : ptr_(std::make_unique<T>(std::move(val))) {}

  Box(const Box& other) : Box(*other.ptr_) {}
  Box(Box&& other) : ptr_(std::move(other.ptr_)) {}

  Box& operator=(const Box& other) {
    ptr_ = std::make_unique<T>(*other.ptr_);
    return *this;
  }

  Box& operator=(Box&& other) {
    ptr_ = std::move(other.ptr_);
    return *this;
  }

  T& operator*() { return *ptr_; }
  const T& operator*() const { return *ptr_; }

  T *operator->() { return ptr_.get(); }
  const T *operator->() const { return ptr_.get(); }

  T *get() { return ptr_.get(); }
  const T *get() const { return ptr_.get(); }

 private:
  Box(std::unique_ptr<T>&& ptr) : ptr_(std::move(ptr)) {}

  std::unique_ptr<T> ptr_;
};

template <typename T> struct is_dynamic : std::false_type {};
template <> struct is_dynamic<Box<DynamicStruct>> : std::true_type {};
template <> struct is_dynamic<Box<DynamicArray>> : std::true_type {};

using AnyField =
    std::variant<uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t, bool,
                 float, double, Box<DynamicArray>, Box<DynamicStruct>>;

static inline AnyField MakeAnyField(const TypeDescriptor *field_type_descriptor) {
  using Type = TypeDescriptor::Type;
  using PrimType = TypeDescriptor::PrimType;

  switch (field_type_descriptor->type()) {
    case Type::kPrimitive:
    case Type::kEnum:
      switch (field_type_descriptor->prim_type()) {
        case PrimType::kUint8:
          return AnyField(std::in_place_type<uint8_t>);
        case PrimType::kUint16:
          return AnyField(std::in_place_type<uint16_t>);
        case PrimType::kUint32:
          return AnyField(std::in_place_type<uint32_t>);
        case PrimType::kUint64:
          return AnyField(std::in_place_type<uint64_t>);
        case PrimType::kInt8:
          return AnyField(std::in_place_type<int8_t>);
        case PrimType::kInt16:
          return AnyField(std::in_place_type<int16_t>);
        case PrimType::kInt32:
          return AnyField(std::in_place_type<int32_t>);
        case PrimType::kInt64:
          return AnyField(std::in_place_type<int64_t>);
        case PrimType::kBool:
          return AnyField(std::in_place_type<bool>);
        case PrimType::kFloat:
          return AnyField(std::in_place_type<float>);
        case PrimType::kDouble:
          return AnyField(std::in_place_type<double>);
      }
    case Type::kArray:
      return Box<DynamicArray>::Make(*field_type_descriptor);
    case Type::kStruct:
      return Box<DynamicStruct>::Make(*field_type_descriptor);
  }

  return {};
}

};  // namespace impl

class DynamicStruct {
 public:
  DynamicStruct(const TypeDescriptor& descriptor) : descriptor_{descriptor} {
    for (const std::unique_ptr<const FieldDescriptor>& field : descriptor.struct_fields()) {
      fields_.emplace(field.get(), impl::MakeAnyField(field->type()));
    }
  }

  template <typename T>
  T& Get(const FieldDescriptor& field_descriptor) {
    impl::AnyField& field = fields_.at(&field_descriptor);

    if constexpr (std::is_same_v<T, DynamicStruct> || std::is_same_v<T, DynamicArray>) {
      return *std::get<impl::Box<T>>(field);
    } else {
      return std::get<T>(field);
    }
  }

  template <typename T>
  T& Get(std::string_view field_name) {
    return Get<T>(*descriptor_[field_name]);
  }

  template <typename T>
  T *GetIf(const FieldDescriptor& field_descriptor) {
    const auto& field_it = fields_.find(&field_descriptor);
    if (field_it == fields_.end()) return nullptr;

    impl::AnyField& field = field_it->second;

    if constexpr (std::is_same_v<T, DynamicStruct> || std::is_same_v<T, DynamicArray>) {
      return *std::get<impl::Box<T>>(field);
    } else {
      return &std::get<T>(field);
    }
  }

  template <typename T>
  T *GetIf(std::string_view field_name) {
    const FieldDescriptor *field = descriptor_[field_name];
    if (!field) return nullptr;
    return GetIf<T>(*field);
  }

  template <typename T>
  T Convert(const FieldDescriptor& field_descriptor) {
    impl::AnyField& field = fields_.at(&field_descriptor);

    return std::visit(
        [](auto&& field) {
          using U = std::decay_t<decltype(field)>;

          if constexpr (impl::is_dynamic<U>::value) {
            throw std::bad_variant_access();
            return T{};
          } else {
            return static_cast<T>(field);
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
          using U = std::decay_t<decltype(field)>;

          if constexpr (impl::is_dynamic<U>::value) {
             throw std::bad_variant_access();
            return std::optional<T>{};
          } else {
            return std::optional<T>{static_cast<T>(field)};
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
  std::unordered_map<const FieldDescriptor *, impl::AnyField> fields_;
  const TypeDescriptor& descriptor_;
};

class DynamicArray {
 public:
  DynamicArray(const TypeDescriptor& descriptor)
      : descriptor_{descriptor}, elems_(descriptor.array_size()) {
    for (size_t i = 0; i < elems_.size(); ++i) {
      elems_[i] = impl::MakeAnyField(descriptor.array_elem_type());
    }
  }

  template <typename T>
  T& Get(size_t i) {
    return *std::get<impl::Box<T>>(elems_[i]).get();
  }

  template <typename T>
  T Convert(size_t i) {
    impl::AnyField& field = elems_[i];

    return std::visit(
        [](auto&& field) {
          using U = std::decay_t<decltype(field)>;

          if constexpr (impl::is_dynamic<U>::value) {
            throw std::bad_variant_access();
            return T{};
          }

          return static_cast<T>(field);
        },
        field);
  }

  size_t size() const { return elems_.size(); };
  const TypeDescriptor& descriptor() const { return descriptor_; }

 private:
  const TypeDescriptor& descriptor_;
  std::vector<impl::AnyField> elems_;
};

}  // namespace ss
