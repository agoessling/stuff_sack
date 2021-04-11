#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "external/unity/src/unity.h"

#include "test/test_message_def.h"

static void TestBitfield(void) {
  TEST_ASSERT_EQUAL_INT(2, sizeof(Bitfield2Bytes));
  TEST_ASSERT_EQUAL_INT(4, sizeof(Bitfield4Bytes));

  BitfieldTest bitfield_test = {
    .bitfield = {
      .field0 = 6,
      .field1 = 27,
      .field2 = 105,
      .field3 = 1,
    },
  };
  BitfieldTest unpacked;

  uint8_t bytes[MP_BITFIELD_TEST_PACKED_SIZE] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00,
    0x00, 0x01, 0x69, 0xde,
  };
  uint8_t packed[MP_BITFIELD_TEST_PACKED_SIZE];

  MpPackBitfieldTest(&bitfield_test, packed);

  TEST_ASSERT_EQUAL_HEX8_ARRAY(&bytes[6], &packed[6], 3);

  memcpy(bytes, packed, 6);
  MpStatus status = MpUnpackBitfieldTest(bytes, &unpacked);
  TEST_ASSERT_EQUAL_INT(kMpStatusSuccess, status);

  TEST_ASSERT_EQUAL_INT(bitfield_test.bitfield.field0, unpacked.bitfield.field0);
  TEST_ASSERT_EQUAL_INT(bitfield_test.bitfield.field1, unpacked.bitfield.field1);
  TEST_ASSERT_EQUAL_INT(bitfield_test.bitfield.field2, unpacked.bitfield.field2);
  TEST_ASSERT_EQUAL_INT(bitfield_test.bitfield.field3, unpacked.bitfield.field3);
}

static void TestEnum(void) {
  TEST_ASSERT_EQUAL_INT(127, kNumEnum1Bytes);
  TEST_ASSERT_EQUAL_INT(7, MP_ENUM1_BYTES_TEST_PACKED_SIZE);

  TEST_ASSERT_EQUAL_INT(128, kNumEnum2Bytes);
  TEST_ASSERT_EQUAL_INT(8, MP_ENUM2_BYTES_TEST_PACKED_SIZE);

  Enum2BytesTest enum_test = {
    .enumeration = kNumEnum2Bytes,
  };
  Enum2BytesTest unpacked;

  uint8_t bytes[MP_ENUM2_BYTES_TEST_PACKED_SIZE] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00,
    0x00, 0x80,
  };
  uint8_t packed[MP_ENUM2_BYTES_TEST_PACKED_SIZE];

  MpPackEnum2BytesTest(&enum_test, packed);

  TEST_ASSERT_EQUAL_HEX8_ARRAY(&bytes[6], &packed[6], 2);

  memcpy(bytes, packed, 6);
  MpStatus status = MpUnpackEnum2BytesTest(bytes, &unpacked);
  TEST_ASSERT_EQUAL_INT(kMpStatusSuccess, status);

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

  uint8_t bytes[MP_PRIMITIVE_TEST_PACKED_SIZE] = {
    0x00, 0x00, 0x00, 0x00, // uid
    0x00, 0x00, // len, 4
    0x01, // uint8, 5
    0x02, 0x01, // uint16, 7
    0x04, 0x03, 0x02, 0x01, // uint32, 9
    0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, // uint64, 13
    0x01, // int8, 21
    0x02, 0x01, // int16, 22
    0x04, 0x03, 0x02, 0x01, // int32, 24
    0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, // int64, 28
    0x01, // bool, 36
    0x40, 0x49, 0x0f, 0xda, // float, 37
    0x40, 0x09, 0x21, 0xFB, 0x4D, 0x12, 0xD8, 0x4A, // double, 41
  };
  uint8_t packed[MP_PRIMITIVE_TEST_PACKED_SIZE];

  MpPackPrimitiveTest(&primitive_test, packed);

  TEST_ASSERT_EQUAL_HEX8_ARRAY(&bytes[6], &packed[6], MP_PRIMITIVE_TEST_PACKED_SIZE - 6);

  memcpy(bytes, packed, 6);
  MpStatus status = MpUnpackPrimitiveTest(bytes, &unpacked);
  TEST_ASSERT_EQUAL_INT(kMpStatusSuccess, status);

  TEST_ASSERT_EQUAL_HEX8(primitive_test.uint8, unpacked.uint8);
  TEST_ASSERT_EQUAL_HEX16(primitive_test.uint16, unpacked.uint16);
  TEST_ASSERT_EQUAL_HEX32(primitive_test.uint32, unpacked.uint32);
  TEST_ASSERT_EQUAL_HEX64(primitive_test.uint64, unpacked.uint64);
  TEST_ASSERT_EQUAL_HEX8(primitive_test.int8, unpacked.int8);
  TEST_ASSERT_EQUAL_HEX16(primitive_test.int16, unpacked.int16);
  TEST_ASSERT_EQUAL_HEX32(primitive_test.int32, unpacked.int32);
  TEST_ASSERT_EQUAL_HEX64(primitive_test.int64, unpacked.int64);
  TEST_ASSERT_EQUAL_INT(primitive_test.boolean, unpacked.boolean);
  TEST_ASSERT_EQUAL_HEX32(primitive_test.float_type, unpacked.float_type);
  TEST_ASSERT_EQUAL_HEX64(primitive_test.double_type, unpacked.double_type);
}

static void TestArray(void) {
  ArrayTest array_test = {
    .array_1d = {
      {.field1 = 0},
      {.field1 = 1},
      {.field1 = 2},
    },
    .array_2d = {
      {
        {.field1 = 0},
        {.field1 = 1},
        {.field1 = 2},
        {.field1 = 3},
        {.field1 = 4},
        {.field1 = 5},
        {.field1 = 6},
        {.field1 = 7},
      },
      {
        {.field1 = 8},
        {.field1 = 9},
        {.field1 = 10},
        {.field1 = 11},
        {.field1 = 12},
        {.field1 = 13},
        {.field1 = 14},
        {.field1 = 15},
      },
    },
  };
  ArrayTest unpacked;

  uint8_t bytes[MP_ARRAY_TEST_PACKED_SIZE] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x01,
    0x00, 0x00, 0x02,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x01,
    0x00, 0x00, 0x02,
    0x00, 0x00, 0x03,
    0x00, 0x00, 0x04,
    0x00, 0x00, 0x05,
    0x00, 0x00, 0x06,
    0x00, 0x00, 0x07,
    0x00, 0x00, 0x08,
    0x00, 0x00, 0x09,
    0x00, 0x00, 0x0a,
    0x00, 0x00, 0x0b,
    0x00, 0x00, 0x0c,
    0x00, 0x00, 0x0d,
    0x00, 0x00, 0x0e,
    0x00, 0x00, 0x0f,
  };
  uint8_t packed[MP_ARRAY_TEST_PACKED_SIZE];

  MpPackArrayTest(&array_test, packed);

  TEST_ASSERT_EQUAL_HEX8_ARRAY(&bytes[6], &packed[6], MP_ARRAY_TEST_PACKED_SIZE - 6);

  memcpy(bytes, packed, 6);
  MpStatus status = MpUnpackArrayTest(bytes, &unpacked);
  TEST_ASSERT_EQUAL_INT(kMpStatusSuccess, status);

  for (int32_t i = 0; i < 2; ++i) {
    TEST_ASSERT_EQUAL_INT(array_test.array_1d[i].field0, unpacked.array_1d[i].field0);
    TEST_ASSERT_EQUAL_INT(array_test.array_1d[i].field1, unpacked.array_1d[i].field1);
  }

  for (int32_t i = 0; i < 2; ++i) {
    for (int32_t j = 0; j < 8; ++j) {
      TEST_ASSERT_EQUAL_INT(array_test.array_2d[i][j].field0,
                            unpacked.array_2d[i][j].field0);
      TEST_ASSERT_EQUAL_INT(array_test.array_2d[i][j].field1,
                            unpacked.array_2d[i][j].field1);
    }
  }
}

static void TestInspectHeader(void) {
  PrimitiveTest primitive_test = {0};
  uint8_t packed[MP_PRIMITIVE_TEST_PACKED_SIZE];

  MpPackPrimitiveTest(&primitive_test, packed);
  TEST_ASSERT_EQUAL_INT(kMpMsgTypePrimitiveTest, MpInspectHeader(packed));

  packed[0] = 0x00;
  packed[1] = 0x00;
  packed[2] = 0x00;
  packed[3] = 0x00;

  TEST_ASSERT_EQUAL_INT(kMpMsgTypeUnknown, MpInspectHeader(packed));
}

static void TestHeaderCheck(void) {
  PrimitiveTest primitive_test = {0};
  uint8_t packed[MP_PRIMITIVE_TEST_PACKED_SIZE];

  MpPackPrimitiveTest(&primitive_test, packed);

  packed[4] = 0x00;
  packed[5] = 0x00;

  TEST_ASSERT_EQUAL_INT(kMpStatusInvalidLen, MpUnpackPrimitiveTest(packed, &primitive_test));

  MpPackPrimitiveTest(&primitive_test, packed);

  packed[0] = 0x00;
  packed[1] = 0x00;
  packed[2] = 0x00;
  packed[3] = 0x00;

  TEST_ASSERT_EQUAL_INT(kMpStatusInvalidUid, MpUnpackPrimitiveTest(packed, &primitive_test));
}

void setUp(void) {}
void tearDown(void) {}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(TestBitfield);
  RUN_TEST(TestEnum);
  RUN_TEST(TestPrimitive);
  RUN_TEST(TestArray);
  RUN_TEST(TestInspectHeader);
  RUN_TEST(TestHeaderCheck);

  return UNITY_END();
}
