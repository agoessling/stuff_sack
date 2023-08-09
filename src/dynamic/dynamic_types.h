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

#include "src/dynamic/packing.h"
#include "src/dynamic/type_descriptors.h"

// These dynamic types were written with expediency as the primary requirement.  They leave a lot to
// be desired in terms of efficiency, speed, and likely ergonomics.  However they should work as
// advertised (see how I set the bar low?) in the corner use-cases in which they are required.

namespace ss {
namespace dynamic {

class DynamicArray;
class DynamicStruct;

namespace impl {

template <typename T>
class Box {
 public:
  template <typename... Args>
  static Box<T> Make(Args&&...args) {
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

template <typename T>
struct is_dynamic : std::false_type {};
template <>
struct is_dynamic<Box<DynamicStruct>> : std::true_type {};
template <>
struct is_dynamic<Box<DynamicArray>> : std::true_type {};

using AnyField = std::variant<uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t,
                              int64_t, bool, float, double, Box<DynamicArray>, Box<DynamicStruct>>;

static inline AnyField MakeAnyField(const TypeDescriptor& field_type_descriptor) {
  using Type = TypeDescriptor::Type;
  using PrimType = TypeDescriptor::PrimType;

  switch (field_type_descriptor.type()) {
    case Type::kPrimitive:
    case Type::kEnum:
      switch (field_type_descriptor.prim_type()) {
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
      return Box<DynamicArray>::Make(field_type_descriptor);
    case Type::kStruct:
    case Type::kBitfield:
      return Box<DynamicStruct>::Make(field_type_descriptor);
  }

  return {};
}

static inline void UnpackToAnyField(AnyField& any_field, const uint8_t *data,
                                    TypeDescriptor::PrimType prim_type) {
  switch (prim_type) {
    case TypeDescriptor::PrimType::kUint8:
      std::get<uint8_t>(any_field) = UnpackBe<uint8_t>(data);
      break;
    case TypeDescriptor::PrimType::kUint16:
      std::get<uint16_t>(any_field) = UnpackBe<uint16_t>(data);
      break;
    case TypeDescriptor::PrimType::kUint32:
      std::get<uint32_t>(any_field) = UnpackBe<uint32_t>(data);
      break;
    case TypeDescriptor::PrimType::kUint64:
      std::get<uint64_t>(any_field) = UnpackBe<uint64_t>(data);
      break;
    case TypeDescriptor::PrimType::kInt8:
      std::get<int8_t>(any_field) = UnpackBe<int8_t>(data);
      break;
    case TypeDescriptor::PrimType::kInt16:
      std::get<int16_t>(any_field) = UnpackBe<int16_t>(data);
      break;
    case TypeDescriptor::PrimType::kInt32:
      std::get<int32_t>(any_field) = UnpackBe<int32_t>(data);
      break;
    case TypeDescriptor::PrimType::kInt64:
      std::get<int64_t>(any_field) = UnpackBe<int64_t>(data);
      break;
    case TypeDescriptor::PrimType::kBool:
      std::get<bool>(any_field) = UnpackBe<bool>(data);
      break;
    case TypeDescriptor::PrimType::kFloat:
      std::get<float>(any_field) = UnpackBe<float>(data);
      break;
    case TypeDescriptor::PrimType::kDouble:
      std::get<double>(any_field) = UnpackBe<double>(data);
      break;
  }
}

}  // namespace impl

class DynamicStruct {
 public:
  DynamicStruct(const TypeDescriptor& descriptor) : descriptor_{descriptor} {
    for (const std::unique_ptr<const FieldDescriptor>& field : descriptor.struct_fields()) {
      fields_.emplace(field.get(), impl::MakeAnyField(field->type()));
    }
  }

  void Unpack(const uint8_t *data);

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
  void UnpackBitfield(const uint8_t *data) {
    for (const std::unique_ptr<const FieldDescriptor>& field : descriptor_.struct_fields()) {
      const TypeDescriptor& field_type = field->type();
      impl::AnyField& any_field = fields_.at(field.get());
      const int bit_offset = field->bit_offset();
      const int bit_size = field->bit_size();

      switch (descriptor_.prim_type()) {
        case TypeDescriptor::PrimType::kUint8: {
          const uint8_t raw_data = UnpackBe<uint8_t>(data);
          switch (field_type.prim_type()) {
            case TypeDescriptor::PrimType::kUint8:
              std::get<uint8_t>(any_field) =
                  dynamic::UnpackBitfield<uint8_t>(raw_data, bit_offset, bit_size);
              break;
            default:
              throw std::runtime_error("Incorrect bitfield field prim_type.");
          }
          break;
        }
        case TypeDescriptor::PrimType::kUint16: {
          const uint16_t raw_data = UnpackBe<uint16_t>(data);
          switch (field_type.prim_type()) {
            case TypeDescriptor::PrimType::kUint8:
              std::get<uint8_t>(any_field) =
                  dynamic::UnpackBitfield<uint8_t>(raw_data, bit_offset, bit_size);
              break;
            case TypeDescriptor::PrimType::kUint16:
              std::get<uint16_t>(any_field) =
                  dynamic::UnpackBitfield<uint16_t>(raw_data, bit_offset, bit_size);
              break;
            default:
              throw std::runtime_error("Incorrect bitfield field prim_type.");
          }
          break;
        }
        case TypeDescriptor::PrimType::kUint32: {
          const uint32_t raw_data = UnpackBe<uint32_t>(data);
          switch (field_type.prim_type()) {
            case TypeDescriptor::PrimType::kUint8:
              std::get<uint8_t>(any_field) =
                  dynamic::UnpackBitfield<uint8_t>(raw_data, bit_offset, bit_size);
              break;
            case TypeDescriptor::PrimType::kUint16:
              std::get<uint16_t>(any_field) =
                  dynamic::UnpackBitfield<uint16_t>(raw_data, bit_offset, bit_size);
              break;
            case TypeDescriptor::PrimType::kUint32:
              std::get<uint32_t>(any_field) =
                  dynamic::UnpackBitfield<uint32_t>(raw_data, bit_offset, bit_size);
              break;
            default:
              throw std::runtime_error("Incorrect bitfield field prim_type.");
          }
          break;
        }
        case TypeDescriptor::PrimType::kUint64: {
          const uint64_t raw_data = UnpackBe<uint64_t>(data);
          switch (field_type.prim_type()) {
            case TypeDescriptor::PrimType::kUint8:
              std::get<uint8_t>(any_field) =
                  dynamic::UnpackBitfield<uint8_t>(raw_data, bit_offset, bit_size);
              break;
            case TypeDescriptor::PrimType::kUint16:
              std::get<uint16_t>(any_field) =
                  dynamic::UnpackBitfield<uint16_t>(raw_data, bit_offset, bit_size);
              break;
            case TypeDescriptor::PrimType::kUint32:
              std::get<uint32_t>(any_field) =
                  dynamic::UnpackBitfield<uint32_t>(raw_data, bit_offset, bit_size);
              break;
            case TypeDescriptor::PrimType::kUint64:
              std::get<uint64_t>(any_field) =
                  dynamic::UnpackBitfield<uint64_t>(raw_data, bit_offset, bit_size);
              break;
            default:
              throw std::runtime_error("Incorrect bitfield field prim_type.");
          }
          break;
        }
        default:
          throw std::runtime_error("Incorrect bitfield prim_type.");
      }
    }
  }

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

  void Unpack(const uint8_t *data) {
    const TypeDescriptor& elem_type = descriptor_.array_elem_type();

    for (impl::AnyField& any_field : elems_) {
      switch (elem_type.type()) {
        case TypeDescriptor::Type::kPrimitive:
        case TypeDescriptor::Type::kEnum:
          impl::UnpackToAnyField(any_field, data, elem_type.prim_type());
          break;
        case TypeDescriptor::Type::kArray:
          std::get<impl::Box<DynamicArray>>(any_field)->Unpack(data);
          break;
        case TypeDescriptor::Type::kStruct:
        case TypeDescriptor::Type::kBitfield:
          std::get<impl::Box<DynamicStruct>>(any_field)->Unpack(data);
          break;
      }

      data += elem_type.packed_size();
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

inline void DynamicStruct::Unpack(const uint8_t *data) {
  if (descriptor_.type() == TypeDescriptor::Type::kBitfield) {
    UnpackBitfield(data);
    return;
  }

  for (const std::unique_ptr<const FieldDescriptor>& field : descriptor_.struct_fields()) {
    const TypeDescriptor& field_type = field->type();
    impl::AnyField& any_field = fields_.at(field.get());

    switch (field_type.type()) {
      case TypeDescriptor::Type::kPrimitive:
      case TypeDescriptor::Type::kEnum:
        impl::UnpackToAnyField(any_field, data, field_type.prim_type());
        break;
      case TypeDescriptor::Type::kArray:
        std::get<impl::Box<DynamicArray>>(any_field)->Unpack(data);
        break;
      case TypeDescriptor::Type::kStruct:
      case TypeDescriptor::Type::kBitfield:
        std::get<impl::Box<DynamicStruct>>(any_field)->Unpack(data);
        break;
    }

    data += field_type.packed_size();
  }
}

enum class UnpackStatus {
  kSuccess,
  kInvalidLen,
  kInvalidUid,
};

static std::pair<std::optional<DynamicStruct>, UnpackStatus> UnpackMessage(
    const uint8_t *data, size_t len, const DescriptorBuilder& types) {
  if (len < 6) return std::make_pair(std::nullopt, UnpackStatus::kInvalidLen);

  DynamicStruct ss_header(*types["SsHeader"]);
  ss_header.Unpack(data);

  const uint16_t msg_len = ss_header.Get<uint16_t>("len");
  if (msg_len != len) return std::make_pair(std::nullopt, UnpackStatus::kInvalidLen);

  const uint32_t msg_uid = ss_header.Get<uint32_t>("uid");

  const TypeDescriptor *msg_type = types.LookupMsgFromUid(msg_uid);
  if (!msg_type) return std::make_pair(std::nullopt, UnpackStatus::kInvalidUid);

  if (msg_len != msg_type->packed_size()) {
    return std::make_pair(std::nullopt, UnpackStatus::kInvalidLen);
  }

  DynamicStruct msg(*msg_type);
  msg.Unpack(data);
  return std::make_pair(msg, UnpackStatus::kSuccess);
}

}  // namespace dynamic
}  // namespace ss
