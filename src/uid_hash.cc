#include "src/uid_hash.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include "src/crc32.h"

namespace ss {

#ifdef PYTHON_LIB
extern "C" {
#endif

uint32_t PrimitiveHash(const char *name, int packed_size) {
  return GetCrc32(std::string(name) + ", " + std::to_string(packed_size));
}

uint32_t ArrayHash(uint32_t type_hash, int array_size) {
  return GetCrc32(std::to_string(type_hash) + ", " + std::to_string(array_size));
}

uint32_t BitfieldFieldHash(const char *name, int bits) {
  return GetCrc32(std::string(name) + ", " + std::to_string(bits));
}

uint32_t BitfieldHash(const char *name, uint32_t *field_uids, size_t field_uids_len) {
  std::string s = name;

  for (size_t i = 0; i < field_uids_len; ++i) {
    s += ", " + std::to_string(field_uids[i]);
  }

  return GetCrc32(s);
}

uint32_t EnumValueHash(const char *name, int value) {
  return GetCrc32(std::string(name) + ", " + std::to_string(value));
}

uint32_t EnumHash(const char *name, uint32_t *value_uids, size_t value_uids_len) {
  std::string s = name;

  for (size_t i = 0; i < value_uids_len; ++i) {
    s += ", " + std::to_string(value_uids[i]);
  }

  return GetCrc32(s);
}

uint32_t StructFieldHash(const char *name, uint32_t type_hash) {
  return GetCrc32(std::string(name) + ", " + std::to_string(type_hash));
}

uint32_t StructHash(const char *name, uint32_t *field_uids, size_t field_uids_len) {
  std::string s = name;

  for (size_t i = 0; i < field_uids_len; ++i) {
    s += ", " + std::to_string(field_uids[i]);
  }

  return GetCrc32(s);
}

#ifdef PYTHON_LIB
}  // extern "C"
#endif

}  // namespace ss
