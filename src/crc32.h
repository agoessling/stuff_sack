#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ss {

// Generate CRC table using the reversed polynomial from CRC-32.
constexpr std::array<uint32_t, 256> GenerateCrc32Table() {
  constexpr uint32_t kPoly = 0xEDB88320;

  std::array<uint32_t, 256> table{};

  for (uint32_t i = 0; i < 256; ++i) {
    uint32_t c = i;
    for (uint32_t j = 0; j < 8; ++j) {
      if (c & 1) {
        c = kPoly ^ (c >> 1);
      } else {
        c >>= 1;
      }
    }
    table[i] = c;
  }

  return table;
}

inline uint32_t GetCrc32(const void *data, size_t len) {
  static constexpr std::array<uint32_t, 256> kCrc32Table = GenerateCrc32Table();

  const uint8_t *buf = static_cast<const uint8_t *>(data);

  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < len; ++i) {
    crc = kCrc32Table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
  }
  return crc ^ 0xFFFFFFFF;
}

template <typename T>
static inline uint32_t GetCrc32(const T& data) {
  return GetCrc32(data.data(), data.size() * sizeof(typename T::value_type));
}

};  // namespace ss
