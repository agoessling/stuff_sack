#pragma once

#include <cstddef>
#include <cstdint>

#ifdef PYTHON_LIB
extern "C" {
#endif

uint32_t PrimitiveHash(const char *name, int packed_size);
uint32_t ArrayHash(uint32_t type_hash, int array_size);
uint32_t BitfieldFieldHash(const char *name, int bits);
uint32_t BitfieldHash(const char *name, uint32_t *field_uids, size_t field_uids_len);
uint32_t EnumValueHash(const char *name, int value);
uint32_t EnumHash(const char *name, uint32_t *value_uids, size_t value_uids_len);
uint32_t StructFieldHash(const char *name, uint32_t type_hash);
uint32_t StructHash(const char *name, uint32_t *field_uids, size_t field_uids_len);

#ifdef PYTHON_LIB
}  // extern "C"
#endif
