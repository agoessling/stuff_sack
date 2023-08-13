#include <variant>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "test/test_message_def.hpp"

using namespace ss;
using namespace testing;

TEST(Bitfield, Packing) {
  EXPECT_EQ(Bitfield2BytesTest::kPackedSize, 8);
  EXPECT_EQ(Bitfield4BytesTest::kPackedSize, 10);

  Bitfield4BytesTest bitfield_test = {
      .bitfield =
          {
              .field0 = 6,
              .field1 = 27,
              .field2 = 264,
          },
  };

  uint8_t bytes[Bitfield4BytesTest::kPackedSize] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0xde,
  };

  {
    uint8_t packed[Bitfield4BytesTest::kPackedSize];
    bitfield_test.Pack(packed);
    // Copy over uid and len.
    memcpy(bytes, packed, 6);

    EXPECT_THAT(packed, ElementsAreArray(bytes));

    Bitfield4BytesTest unpacked;
    Status status = unpacked.Unpack(bytes);
    EXPECT_EQ(status, Status::kSuccess);
    EXPECT_EQ(unpacked.bitfield.field0, bitfield_test.bitfield.field0);
    EXPECT_EQ(unpacked.bitfield.field1, bitfield_test.bitfield.field1);
    EXPECT_EQ(unpacked.bitfield.field2, bitfield_test.bitfield.field2);
  }
  {
    const auto packed = bitfield_test.Pack();
    EXPECT_THAT(*packed, ElementsAreArray(bytes));

    const auto [unpacked, status] = Bitfield4BytesTest::UnpackNew(bytes);
    EXPECT_EQ(status, Status::kSuccess);
    EXPECT_EQ(unpacked.bitfield.field0, bitfield_test.bitfield.field0);
    EXPECT_EQ(unpacked.bitfield.field1, bitfield_test.bitfield.field1);
    EXPECT_EQ(unpacked.bitfield.field2, bitfield_test.bitfield.field2);
  }
}

TEST(Enum, Packing) {
  EXPECT_EQ(Enum1BytesTest::kPackedSize, 7);
  EXPECT_EQ(Enum2BytesTest::kPackedSize, 8);

  Enum2BytesTest enum_test = {
      .enumeration = Enum2Bytes::kValue127,
  };

  uint8_t bytes[Enum2BytesTest::kPackedSize] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F,
  };

  {
    uint8_t packed[Enum2BytesTest::kPackedSize];
    enum_test.Pack(packed);
    // Copy over uid and len.
    memcpy(bytes, packed, 6);

    EXPECT_THAT(packed, ElementsAreArray(bytes));

    Enum2BytesTest unpacked;
    Status status = unpacked.Unpack(bytes);
    EXPECT_EQ(status, Status::kSuccess);
    EXPECT_EQ(unpacked.enumeration, enum_test.enumeration);
  }
  {
    const auto packed = enum_test.Pack();
    EXPECT_THAT(*packed, ElementsAreArray(bytes));

    const auto [unpacked, status] = Enum2BytesTest::UnpackNew(bytes);
    EXPECT_EQ(status, Status::kSuccess);
    EXPECT_EQ(unpacked.enumeration, enum_test.enumeration);
  }
}

TEST(Primitive, Packing) {
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

  uint8_t bytes[PrimitiveTest::kPackedSize] = {
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

  {
    uint8_t packed[PrimitiveTest::kPackedSize];
    primitive_test.Pack(packed);
    // Copy over uid and len.
    memcpy(bytes, packed, 6);

    EXPECT_THAT(packed, ElementsAreArray(bytes));

    PrimitiveTest unpacked;
    Status status = unpacked.Unpack(bytes);
    EXPECT_EQ(status, Status::kSuccess);
    EXPECT_EQ(unpacked.uint8, primitive_test.uint8);
    EXPECT_EQ(unpacked.uint16, primitive_test.uint16);
    EXPECT_EQ(unpacked.uint32, primitive_test.uint32);
    EXPECT_EQ(unpacked.uint64, primitive_test.uint64);
    EXPECT_EQ(unpacked.int8, primitive_test.int8);
    EXPECT_EQ(unpacked.int16, primitive_test.int16);
    EXPECT_EQ(unpacked.int32, primitive_test.int32);
    EXPECT_EQ(unpacked.int64, primitive_test.int64);
    EXPECT_EQ(unpacked.boolean, primitive_test.boolean);
    EXPECT_EQ(unpacked.float_type, primitive_test.float_type);
    EXPECT_EQ(unpacked.double_type, primitive_test.double_type);
  }
  {
    const auto packed = primitive_test.Pack();
    EXPECT_THAT(*packed, ElementsAreArray(bytes));

    const auto [unpacked, status] = PrimitiveTest::UnpackNew(bytes);
    EXPECT_EQ(status, Status::kSuccess);
    EXPECT_EQ(unpacked.uint8, primitive_test.uint8);
    EXPECT_EQ(unpacked.uint16, primitive_test.uint16);
    EXPECT_EQ(unpacked.uint32, primitive_test.uint32);
    EXPECT_EQ(unpacked.uint64, primitive_test.uint64);
    EXPECT_EQ(unpacked.int8, primitive_test.int8);
    EXPECT_EQ(unpacked.int16, primitive_test.int16);
    EXPECT_EQ(unpacked.int32, primitive_test.int32);
    EXPECT_EQ(unpacked.int64, primitive_test.int64);
    EXPECT_EQ(unpacked.boolean, primitive_test.boolean);
    EXPECT_EQ(unpacked.float_type, primitive_test.float_type);
    EXPECT_EQ(unpacked.double_type, primitive_test.double_type);
  }
}

TEST(Array, Packing) {
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

  uint8_t bytes[ArrayTest::kPackedSize] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00,
      0x03, 0x00, 0x00, 0x04, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
      0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0x00, 0x05,
  };

  {
    uint8_t packed[ArrayTest::kPackedSize];
    array_test.Pack(packed);
    // Copy over uid and len.
    memcpy(bytes, packed, 6);

    EXPECT_THAT(packed, ElementsAreArray(bytes));

    ArrayTest unpacked;
    Status status = unpacked.Unpack(bytes);
    EXPECT_EQ(status, Status::kSuccess);
    EXPECT_EQ(unpacked.array_1d[0].field1, array_test.array_1d[0].field1);
    EXPECT_EQ(unpacked.array_1d[1].field1, array_test.array_1d[1].field1);
    EXPECT_EQ(unpacked.array_1d[2].field1, array_test.array_1d[2].field1);
    EXPECT_EQ(unpacked.array_2d[0][0].field1, array_test.array_2d[0][0].field1);
    EXPECT_EQ(unpacked.array_2d[0][1].field1, array_test.array_2d[0][1].field1);
    EXPECT_EQ(unpacked.array_2d[0][2].field1, array_test.array_2d[0][2].field1);
    EXPECT_EQ(unpacked.array_2d[1][0].field1, array_test.array_2d[1][0].field1);
    EXPECT_EQ(unpacked.array_2d[1][1].field1, array_test.array_2d[1][1].field1);
    EXPECT_EQ(unpacked.array_2d[1][2].field1, array_test.array_2d[1][2].field1);
    EXPECT_EQ(unpacked.array_3d[0][0][0].field1, array_test.array_3d[0][0][0].field1);
    EXPECT_EQ(unpacked.array_3d[0][0][1].field1, array_test.array_3d[0][0][1].field1);
    EXPECT_EQ(unpacked.array_3d[0][0][2].field1, array_test.array_3d[0][0][2].field1);
    EXPECT_EQ(unpacked.array_3d[0][1][0].field1, array_test.array_3d[0][1][0].field1);
    EXPECT_EQ(unpacked.array_3d[0][1][1].field1, array_test.array_3d[0][1][1].field1);
    EXPECT_EQ(unpacked.array_3d[0][1][2].field1, array_test.array_3d[0][1][2].field1);
  }
  {
    const auto packed = array_test.Pack();
    EXPECT_THAT(*packed, ElementsAreArray(bytes));

    const auto [unpacked, status] = ArrayTest::UnpackNew(bytes);
    EXPECT_EQ(status, Status::kSuccess);
    EXPECT_EQ(unpacked.array_1d[0].field1, array_test.array_1d[0].field1);
    EXPECT_EQ(unpacked.array_1d[1].field1, array_test.array_1d[1].field1);
    EXPECT_EQ(unpacked.array_1d[2].field1, array_test.array_1d[2].field1);
    EXPECT_EQ(unpacked.array_2d[0][0].field1, array_test.array_2d[0][0].field1);
    EXPECT_EQ(unpacked.array_2d[0][1].field1, array_test.array_2d[0][1].field1);
    EXPECT_EQ(unpacked.array_2d[0][2].field1, array_test.array_2d[0][2].field1);
    EXPECT_EQ(unpacked.array_2d[1][0].field1, array_test.array_2d[1][0].field1);
    EXPECT_EQ(unpacked.array_2d[1][1].field1, array_test.array_2d[1][1].field1);
    EXPECT_EQ(unpacked.array_2d[1][2].field1, array_test.array_2d[1][2].field1);
    EXPECT_EQ(unpacked.array_3d[0][0][0].field1, array_test.array_3d[0][0][0].field1);
    EXPECT_EQ(unpacked.array_3d[0][0][1].field1, array_test.array_3d[0][0][1].field1);
    EXPECT_EQ(unpacked.array_3d[0][0][2].field1, array_test.array_3d[0][0][2].field1);
    EXPECT_EQ(unpacked.array_3d[0][1][0].field1, array_test.array_3d[0][1][0].field1);
    EXPECT_EQ(unpacked.array_3d[0][1][1].field1, array_test.array_3d[0][1][1].field1);
    EXPECT_EQ(unpacked.array_3d[0][1][2].field1, array_test.array_3d[0][1][2].field1);
  }
}

TEST(Alias, Packing) {
  AliasTest alias_test = {
      .position = {1.2f, 2.3f, 3.4f},
      .velocity = {4.5f, 5.6f, 6.7f},
  };
  AliasTest unpacked;

  uint8_t bytes[AliasTest::kPackedSize] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x99, 0x99, 0x9a, 0x40, 0x13, 0x33, 0x33, 0x40,
      0x59, 0x99, 0x9a, 0x40, 0x90, 0x00, 0x00, 0x40, 0xb3, 0x33, 0x33, 0x40, 0xd6, 0x66, 0x66,
  };
  uint8_t packed[AliasTest::kPackedSize];

  alias_test.Pack(packed);
  // Copy over uid and len.
  memcpy(bytes, packed, 6);

  EXPECT_THAT(packed, ElementsAreArray(bytes));

  Status status = unpacked.Unpack(bytes);
  EXPECT_EQ(status, Status::kSuccess);
  EXPECT_EQ(unpacked.position.x, alias_test.position.x);
  EXPECT_EQ(unpacked.position.y, alias_test.position.y);
  EXPECT_EQ(unpacked.position.z, alias_test.position.z);
  EXPECT_EQ(unpacked.velocity.x, alias_test.velocity.x);
  EXPECT_EQ(unpacked.velocity.y, alias_test.velocity.y);
  EXPECT_EQ(unpacked.velocity.z, alias_test.velocity.z);
}

TEST(UnpackMessage, InspectHeader) {
  EXPECT_EQ(kHeaderPackedSize, 6);

  {
    const uint8_t header[kHeaderPackedSize] = {};
    EXPECT_EQ(InspectHeader(header), MsgType::kUnknown);
  }
  {
    const auto packed = PrimitiveTest{}.Pack();
    EXPECT_EQ(InspectHeader(packed->data()), MsgType::kPrimitiveTest);
  }
}

TEST(UnpackMessage, Variant) {
  {
    const uint8_t bytes[4] = {};
    const auto [msg, status] = UnpackMessage(bytes, sizeof(bytes));
    EXPECT_EQ(status, Status::kInvalidLen);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(msg));
  }
  {
    auto packed = PrimitiveTest{}.Pack();
    (*packed)[0] = 0x00;
    const auto [msg, status] = UnpackMessage(packed->data(), packed->size());
    EXPECT_EQ(status, Status::kInvalidUid);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(msg));
  }
  {
    PrimitiveTest primitive_test = {.int8 = -55};
    auto packed = primitive_test.Pack();
    const auto [msg, status] = UnpackMessage(packed->data(), packed->size());
    EXPECT_EQ(status, Status::kSuccess);
    EXPECT_TRUE(std::holds_alternative<PrimitiveTest>(msg));
    EXPECT_EQ(std::get<PrimitiveTest>(msg).int8, primitive_test.int8);
  }
}

TEST(MessageDispatcher, Unpack) {
  MessageDispatcher dispatcher;

  PrimitiveTest primitive_test = {.int8 = -55};
  Enum2BytesTest enum_test = {.enumeration = Enum2Bytes::kValue127};

  PrimitiveTest unpacked_primitive_test;
  Enum2BytesTest unpacked_enum_test;

  dispatcher.AddCallback<PrimitiveTest>(
      [&unpacked_primitive_test](const PrimitiveTest& msg) { unpacked_primitive_test = msg; });
  dispatcher.AddCallback<Enum2BytesTest>(
      [&unpacked_enum_test](const Enum2BytesTest& msg) { unpacked_enum_test = msg; });

  dispatcher.Unpack(primitive_test.Pack()->data(), PrimitiveTest::kPackedSize);
  dispatcher.Unpack(enum_test.Pack()->data(), Enum2BytesTest::kPackedSize);

  EXPECT_EQ(unpacked_primitive_test.int8, primitive_test.int8);
  EXPECT_EQ(unpacked_enum_test.enumeration, enum_test.enumeration);
}
