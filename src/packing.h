#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace ss {

namespace impl {

template <typename>
inline constexpr bool always_false_v = false;

};  // namespace impl

template <typename T>
static inline T UnpackBe(const uint8_t *data) {
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
    static_assert(impl::always_false_v<T>);
  }

  return value;
}

template <typename T>
static inline void PackBe(T data, uint8_t *buf) {
  if constexpr (sizeof(data) == 1) {
    uint8_t raw_value;
    memcpy(&raw_value, &data, sizeof(data));
    buf[0] = raw_value;
  } else if constexpr (sizeof(data) == 2) {
    uint16_t raw_value;
    memcpy(&raw_value, &data, sizeof(data));
    buf[0] = raw_value >> 8;
    buf[1] = raw_value >> 0;
  } else if constexpr (sizeof(data) == 4) {
    uint32_t raw_value;
    memcpy(&raw_value, &data, sizeof(data));
    buf[0] = raw_value >> 24;
    buf[1] = raw_value >> 16;
    buf[2] = raw_value >> 8;
    buf[3] = raw_value >> 0;
  } else if constexpr (sizeof(data) == 8) {
    uint64_t raw_value;
    memcpy(&raw_value, &data, sizeof(data));
    buf[0] = raw_value >> 56;
    buf[1] = raw_value >> 48;
    buf[2] = raw_value >> 40;
    buf[3] = raw_value >> 32;
    buf[4] = raw_value >> 24;
    buf[5] = raw_value >> 16;
    buf[6] = raw_value >> 8;
    buf[7] = raw_value >> 0;
  } else {
    static_assert(impl::always_false_v<T>);
  }
}

template <typename T, typename U>
static inline T UnpackBitfield(U data, size_t bit_offset, size_t bit_size) {
  static_assert(std::is_unsigned_v<U>);
  static_assert(sizeof(T) <= sizeof(U));

  const U mask = (U{1} << bit_size) - 1;
  T value = (data >> bit_offset) & mask;

  if constexpr (std::is_signed_v<T>) {
    value = static_cast<T>((value << (8 * sizeof(T) - bit_size))) >> (8 * sizeof(T) - bit_size);
  }

  return value;
}

template <typename T, typename U>
static inline void PackBitfield(T data, U *dest, size_t bit_offset, size_t bit_size) {
  static_assert(std::is_unsigned_v<U>);
  static_assert(sizeof(T) <= sizeof(U));

  const std::make_unsigned_t<T> mask = (std::make_unsigned_t<T>{1} << bit_size) - 1;
  const std::make_unsigned_t<T> raw_data = data & mask;

  *dest &= ~(mask << bit_offset);
  *dest |= raw_data << bit_offset;
}

};  // namespace ss
