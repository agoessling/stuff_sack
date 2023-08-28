#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "external/unity/src/unity.h"

#include "test/external_c_vector3f.h"
#include "test/test_message_def.h"

static void TestBitfield(void) {
  TEST_ASSERT_EQUAL_INT(8, SS_BITFIELD2_BYTES_TEST_PACKED_SIZE);
  TEST_ASSERT_EQUAL_INT(10, SS_BITFIELD4_BYTES_TEST_PACKED_SIZE);

  Bitfield4BytesTest bitfield_test = {
      .bitfield =
          {
              .field0 = 6,
              .field1 = 27,
              .field2 = 264,
          },
  };
  Bitfield4BytesTest unpacked;

  uint8_t bytes[SS_BITFIELD4_BYTES_TEST_PACKED_SIZE] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0xde,
  };
  uint8_t packed[SS_BITFIELD4_BYTES_TEST_PACKED_SIZE];

  SsPackBitfield4BytesTest(&bitfield_test, packed);

  TEST_ASSERT_EQUAL_HEX8_ARRAY(&bytes[6], &packed[6], 3);

  memcpy(bytes, packed, 6);
  SsStatus status = SsUnpackBitfield4BytesTest(bytes, &unpacked);
  TEST_ASSERT_EQUAL_INT(kSsStatusSuccess, status);

  TEST_ASSERT_EQUAL_INT(bitfield_test.bitfield.field0, unpacked.bitfield.field0);
  TEST_ASSERT_EQUAL_INT(bitfield_test.bitfield.field1, unpacked.bitfield.field1);
  TEST_ASSERT_EQUAL_INT(bitfield_test.bitfield.field2, unpacked.bitfield.field2);
}

static void TestEnum(void) {
  TEST_ASSERT_EQUAL_INT(127, kNumEnum1Bytes);
  TEST_ASSERT_EQUAL_INT(7, SS_ENUM1_BYTES_TEST_PACKED_SIZE);

  TEST_ASSERT_EQUAL_INT(128, kNumEnum2Bytes);
  TEST_ASSERT_EQUAL_INT(8, SS_ENUM2_BYTES_TEST_PACKED_SIZE);

  TEST_ASSERT_EQUAL_INT(sizeof(int), sizeof(Enum1Bytes));
  TEST_ASSERT_EQUAL_INT(sizeof(int), sizeof(Enum2Bytes));

  Enum2BytesTest enum_test = {
      .enumeration = kNumEnum2Bytes,
  };
  Enum2BytesTest unpacked;

  uint8_t bytes[SS_ENUM2_BYTES_TEST_PACKED_SIZE] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
  };
  uint8_t packed[SS_ENUM2_BYTES_TEST_PACKED_SIZE];

  SsPackEnum2BytesTest(&enum_test, packed);

  TEST_ASSERT_EQUAL_HEX8_ARRAY(&bytes[6], &packed[6], 2);

  memcpy(bytes, packed, 6);
  SsStatus status = SsUnpackEnum2BytesTest(bytes, &unpacked);
  TEST_ASSERT_EQUAL_INT(kSsStatusSuccess, status);

  TEST_ASSERT_EQUAL_INT(enum_test.enumeration, unpacked.enumeration);
}

static void TestPrimitive(void) {
  PrimitiveTest primitive_test = {
      .uint8 = 0x01,
      .uint16 = 0x0201,
      .uint32 = 0x04030201,
      .uint64 = 0x0807060504030201,
      .int8 = 0x01,
      .int16 = 0x0201,
      .int32 = 0x04030201,
      .int64 = 0x0807060504030201,
      .boolean = true,
      .float_type = 3.1415926,
      .double_type = 3.1415926,
  };
  PrimitiveTest unpacked;

  uint8_t bytes[SS_PRIMITIVE_TEST_PACKED_SIZE] = {
      0x00, 0x00, 0x00, 0x00,  // uid
      0x00, 0x00,  // len, 4
      0x01,  // uint8, 5
      0x02, 0x01,  // uint16, 7
      0x04, 0x03, 0x02, 0x01,  // uint32, 9
      0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,  // uint64, 13
      0x01,  // int8, 21
      0x02, 0x01,  // int16, 22
      0x04, 0x03, 0x02, 0x01,  // int32, 24
      0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,  // int64, 28
      0x01,  // bool, 36
      0x40, 0x49, 0x0f, 0xda,  // float, 37
      0x40, 0x09, 0x21, 0xFB, 0x4D, 0x12, 0xD8, 0x4A,  // double, 41
  };
  uint8_t packed[SS_PRIMITIVE_TEST_PACKED_SIZE];

  SsPackPrimitiveTest(&primitive_test, packed);

  TEST_ASSERT_EQUAL_HEX8_ARRAY(&bytes[6], &packed[6], SS_PRIMITIVE_TEST_PACKED_SIZE - 6);

  memcpy(bytes, packed, 6);
  SsStatus status = SsUnpackPrimitiveTest(bytes, &unpacked);
  TEST_ASSERT_EQUAL_INT(kSsStatusSuccess, status);

  TEST_ASSERT_EQUAL_HEX8(primitive_test.uint8, unpacked.uint8);
  TEST_ASSERT_EQUAL_HEX16(primitive_test.uint16, unpacked.uint16);
  TEST_ASSERT_EQUAL_HEX32(primitive_test.uint32, unpacked.uint32);
  TEST_ASSERT_EQUAL_HEX64(primitive_test.uint64, unpacked.uint64);
  TEST_ASSERT_EQUAL_HEX8(primitive_test.int8, unpacked.int8);
  TEST_ASSERT_EQUAL_HEX16(primitive_test.int16, unpacked.int16);
  TEST_ASSERT_EQUAL_HEX32(primitive_test.int32, unpacked.int32);
  TEST_ASSERT_EQUAL_HEX64(primitive_test.int64, unpacked.int64);
  TEST_ASSERT_EQUAL_INT(primitive_test.boolean, unpacked.boolean);
  TEST_ASSERT_EQUAL_FLOAT(primitive_test.float_type, unpacked.float_type);
  TEST_ASSERT_EQUAL_DOUBLE(primitive_test.double_type, unpacked.double_type);
}

static void TestArray(void) {
  ArrayTest array_test = {.array_1d =
                              {
                                  {.field1 = 0},
                                  {.field1 = 1},
                                  {.field1 = 2},
                              },
                          .array_2d =
                              {
                                  {
                                      {.field1 = 0},
                                      {.field1 = 1},
                                      {.field1 = 2},
                                  },
                                  {
                                      {.field1 = 3},
                                      {.field1 = 4},
                                      {.field1 = 5},
                                  },
                              },
                          .array_3d = {
                              {
                                  {
                                      {.field1 = 0},
                                      {.field1 = 1},
                                      {.field1 = 2},
                                  },
                                  {
                                      {.field1 = 3},
                                      {.field1 = 4},
                                      {.field1 = 5},
                                  },
                              },
                          }};
  ArrayTest unpacked;

  uint8_t bytes[SS_ARRAY_TEST_PACKED_SIZE] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00,
      0x03, 0x00, 0x00, 0x04, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
      0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0x00, 0x05,
  };
  uint8_t packed[SS_ARRAY_TEST_PACKED_SIZE];

  SsPackArrayTest(&array_test, packed);

  TEST_ASSERT_EQUAL_HEX8_ARRAY(&bytes[6], &packed[6], SS_ARRAY_TEST_PACKED_SIZE - 6);

  memcpy(bytes, packed, 6);
  SsStatus status = SsUnpackArrayTest(bytes, &unpacked);
  TEST_ASSERT_EQUAL_INT(kSsStatusSuccess, status);

  for (int32_t i = 0; i < 3; ++i) {
    TEST_ASSERT_EQUAL_INT(array_test.array_1d[i].field0, unpacked.array_1d[i].field0);
    TEST_ASSERT_EQUAL_INT(array_test.array_1d[i].field1, unpacked.array_1d[i].field1);
  }

  for (int32_t i = 0; i < 3; ++i) {
    for (int32_t j = 0; j < 2; ++j) {
      TEST_ASSERT_EQUAL_INT(array_test.array_2d[i][j].field0, unpacked.array_2d[i][j].field0);
      TEST_ASSERT_EQUAL_INT(array_test.array_2d[i][j].field1, unpacked.array_2d[i][j].field1);
    }
  }
  for (int32_t i = 0; i < 3; ++i) {
    for (int32_t j = 0; j < 2; ++j) {
      for (int32_t k = 0; k < 1; ++k) {
        TEST_ASSERT_EQUAL_INT(array_test.array_3d[k][j][i].field0,
                              unpacked.array_3d[k][j][i].field0);
        TEST_ASSERT_EQUAL_INT(array_test.array_3d[k][j][i].field1,
                              unpacked.array_3d[k][j][i].field1);
      }
    }
  }
}

static void TestAliasing(void) {
  AliasTest alias_test = {
      .position = {1.2f, 2.3f, 3.4f},
      .velocity = {4.5f, 5.6f, 6.7f},
  };
  AliasTest unpacked;

  uint8_t bytes[SS_ALIAS_TEST_PACKED_SIZE] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x99, 0x99, 0x9a, 0x40, 0x13, 0x33, 0x33, 0x40,
      0x59, 0x99, 0x9a, 0x40, 0x90, 0x00, 0x00, 0x40, 0xb3, 0x33, 0x33, 0x40, 0xd6, 0x66, 0x66,
  };
  uint8_t packed[SS_ALIAS_TEST_PACKED_SIZE];

  SsPackAliasTest(&alias_test, packed);

  TEST_ASSERT_EQUAL_HEX8_ARRAY(&bytes[6], &packed[6], SS_ALIAS_TEST_PACKED_SIZE - 6);

  memcpy(bytes, packed, 6);
  SsStatus status = SsUnpackAliasTest(bytes, &unpacked);
  TEST_ASSERT_EQUAL_INT(kSsStatusSuccess, status);

  TEST_ASSERT_EQUAL_FLOAT(alias_test.position.t, unpacked.position.t);
  TEST_ASSERT_EQUAL_FLOAT(alias_test.position.u, unpacked.position.u);
  TEST_ASSERT_EQUAL_FLOAT(alias_test.position.v, unpacked.position.v);
  TEST_ASSERT_EQUAL_FLOAT(alias_test.velocity.t, unpacked.velocity.t);
  TEST_ASSERT_EQUAL_FLOAT(alias_test.velocity.u, unpacked.velocity.u);
  TEST_ASSERT_EQUAL_FLOAT(alias_test.velocity.v, unpacked.velocity.v);
}

static void TestInspectHeader(void) {
  PrimitiveTest primitive_test = {0};
  uint8_t packed[SS_PRIMITIVE_TEST_PACKED_SIZE];

  SsPackPrimitiveTest(&primitive_test, packed);
  TEST_ASSERT_EQUAL_INT(kSsMsgTypePrimitiveTest, SsInspectHeader(packed));

  packed[0] = 0x00;
  packed[1] = 0x00;
  packed[2] = 0x00;
  packed[3] = 0x00;

  TEST_ASSERT_EQUAL_INT(kSsMsgTypeUnknown, SsInspectHeader(packed));
}

static void TestHeaderCheck(void) {
  PrimitiveTest primitive_test = {0};
  uint8_t packed[SS_PRIMITIVE_TEST_PACKED_SIZE];

  SsPackPrimitiveTest(&primitive_test, packed);

  packed[4] = 0x00;
  packed[5] = 0x00;

  TEST_ASSERT_EQUAL_INT(kSsStatusInvalidLen, SsUnpackPrimitiveTest(packed, &primitive_test));

  SsPackPrimitiveTest(&primitive_test, packed);

  packed[0] = 0x00;
  packed[1] = 0x00;
  packed[2] = 0x00;
  packed[3] = 0x00;

  TEST_ASSERT_EQUAL_INT(kSsStatusInvalidUid, SsUnpackPrimitiveTest(packed, &primitive_test));
}

static void TestLogging(void) {
  const char *tmp_dir = getenv("TEST_TMPDIR");
  if (!tmp_dir) {
    TEST_FAIL_MESSAGE("Could not get writable directory from TEST_TMPDIR");
  }

  const char filename[] = "test_log.ss";

  char *path = malloc(strlen(tmp_dir) + strlen(filename) + 2);  // Separator plus null character.
  sprintf(path, "%s/%s", tmp_dir, filename);

  FILE *const file = fopen(path, "w+");
  free(path);

  if (!file) {
    perror("Could not open temporary log file");
    TEST_FAIL();
  }

  TEST_ASSERT_GREATER_THAN(0, SsWriteLogHeader(file));

  PrimitiveTest primitive_test = {.int8 = 1};
  Enum1BytesTest enum_1_bytes_test = {.enumeration = kEnum1BytesValue3};
  SsLogPrimitiveTest(file, &primitive_test);
  primitive_test.int8++;
  SsLogPrimitiveTest(file, &primitive_test);
  SsLogEnum1BytesTest(file, &enum_1_bytes_test);
  primitive_test.int8++;
  SsLogPrimitiveTest(file, &primitive_test);

  rewind(file);

  const int delim_pos = SsFindLogDelimiter(file);
  TEST_ASSERT_GREATER_THAN(0, delim_pos);

  fseek(file, delim_pos - sizeof(kSsLogDelimiter) + 1, SEEK_SET);

  char buf[sizeof(kSsLogDelimiter) - 1];
  int ret = fread(buf, 1, sizeof(buf), file);

  TEST_ASSERT_EQUAL(sizeof(buf), ret);
  TEST_ASSERT_EQUAL_CHAR_ARRAY(kSsLogDelimiter, buf, sizeof(buf));

  uint8_t primitive_buf[SS_PRIMITIVE_TEST_PACKED_SIZE];
  uint8_t enum_buf[SS_ENUM1_BYTES_TEST_PACKED_SIZE];

  ret = fread(primitive_buf, 1, sizeof(primitive_buf), file);
  TEST_ASSERT_EQUAL(sizeof(primitive_buf), ret);
  TEST_ASSERT_EQUAL(kSsStatusSuccess, SsUnpackPrimitiveTest(primitive_buf, &primitive_test));
  TEST_ASSERT_EQUAL(1, primitive_test.int8);

  ret = fread(primitive_buf, 1, sizeof(primitive_buf), file);
  TEST_ASSERT_EQUAL(sizeof(primitive_buf), ret);
  TEST_ASSERT_EQUAL(kSsStatusSuccess, SsUnpackPrimitiveTest(primitive_buf, &primitive_test));
  TEST_ASSERT_EQUAL(2, primitive_test.int8);

  ret = fread(enum_buf, 1, sizeof(enum_buf), file);
  TEST_ASSERT_EQUAL(sizeof(enum_buf), ret);
  TEST_ASSERT_EQUAL(kSsStatusSuccess, SsUnpackEnum1BytesTest(enum_buf, &enum_1_bytes_test));
  TEST_ASSERT_EQUAL(kEnum1BytesValue3, enum_1_bytes_test.enumeration);

  ret = fread(primitive_buf, 1, sizeof(primitive_buf), file);
  TEST_ASSERT_EQUAL(sizeof(primitive_buf), ret);
  TEST_ASSERT_EQUAL(kSsStatusSuccess, SsUnpackPrimitiveTest(primitive_buf, &primitive_test));
  TEST_ASSERT_EQUAL(3, primitive_test.int8);

  TEST_ASSERT_EQUAL(EOF, fgetc(file));
  TEST_ASSERT_EQUAL(1, feof(file));

  fclose(file);
}

void setUp(void) {}
void tearDown(void) {}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(TestBitfield);
  RUN_TEST(TestEnum);
  RUN_TEST(TestPrimitive);
  RUN_TEST(TestArray);
  RUN_TEST(TestAliasing);
  RUN_TEST(TestInspectHeader);
  RUN_TEST(TestHeaderCheck);
  RUN_TEST(TestLogging);

  return UNITY_END();
}
