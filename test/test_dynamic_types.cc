#include <optional>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "src/dynamic_types.h"
#include "src/type_descriptors.h"

using namespace ss;
using namespace testing;

using PrimType = TypeDescriptor::PrimType;
using Type = TypeDescriptor::Type;

const std::string kYamlFile = "test/test_message_spec.yaml";

TEST(DynamicStruct, FieldGetAccess) {
  DescriptorBuilder types = DescriptorBuilder::FromFile(kYamlFile);
  ASSERT_THAT(types.types(), Contains(Key("PrimitiveTest")));

  const TypeDescriptor& primitive_test = *types["PrimitiveTest"];

  DynamicStruct structure(primitive_test);
  structure.Get<DynamicStruct>("ss_header").Get<uint32_t>("uid") = 505;
  structure.Get<DynamicStruct>("ss_header").Get<uint16_t>("len") = 50;
  structure.Get<uint8_t>("uint8") = 1;
  structure.Get<uint16_t>("uint16") = 2;
  structure.Get<uint32_t>("uint32") = 3;
  structure.Get<uint64_t>("uint64") = 4;
  structure.Get<int8_t>("int8") = 5;
  structure.Get<int16_t>("int16") = 6;
  structure.Get<int32_t>("int32") = 7;
  structure.Get<int64_t>("int64") = 8;
  structure.Get<bool>("boolean") = true;
  structure.Get<float>("float_type") = 10.1f;
  structure.Get<double>("double_type") = 11.1;

  EXPECT_EQ(structure.Get<DynamicStruct>("ss_header").Get<uint32_t>("uid"), 505);
  EXPECT_EQ(structure.Get<DynamicStruct>("ss_header").Get<uint16_t>("len"), 50);
  EXPECT_EQ(structure.Get<uint8_t>("uint8"), 1);
  EXPECT_EQ(structure.Get<uint16_t>("uint16"), 2);
  EXPECT_EQ(structure.Get<uint32_t>("uint32"), 3);
  EXPECT_EQ(structure.Get<uint64_t>("uint64"), 4);
  EXPECT_EQ(structure.Get<int8_t>("int8"), 5);
  EXPECT_EQ(structure.Get<int16_t>("int16"), 6);
  EXPECT_EQ(structure.Get<int32_t>("int32"), 7);
  EXPECT_EQ(structure.Get<int64_t>("int64"), 8);
  EXPECT_EQ(structure.Get<bool>("boolean"), true);
  EXPECT_FLOAT_EQ(structure.Get<float>("float_type"), 10.1f);
  EXPECT_DOUBLE_EQ(structure.Get<double>("double_type"), 11.1);
}

TEST(DynamicStruct, FieldConvertAccess) {
  DescriptorBuilder types = DescriptorBuilder::FromFile(kYamlFile);
  ASSERT_THAT(types.types(), Contains(Key("PrimitiveTest")));

  const TypeDescriptor& primitive_test = *types["PrimitiveTest"];

  DynamicStruct structure(primitive_test);
  structure.Get<DynamicStruct>("ss_header").Get<uint32_t>("uid") = 505;
  structure.Get<DynamicStruct>("ss_header").Get<uint16_t>("len") = 50;
  structure.Get<uint8_t>("uint8") = 1;
  structure.Get<uint16_t>("uint16") = 2;
  structure.Get<uint32_t>("uint32") = 3;
  structure.Get<uint64_t>("uint64") = 4;
  structure.Get<int8_t>("int8") = 5;
  structure.Get<int16_t>("int16") = 6;
  structure.Get<int32_t>("int32") = 7;
  structure.Get<int64_t>("int64") = 8;
  structure.Get<bool>("boolean") = true;
  structure.Get<float>("float_type") = 10.1f;
  structure.Get<double>("double_type") = 11.1;

  EXPECT_EQ(structure.Get<DynamicStruct>("ss_header").Convert<uint8_t>("uid"), 249);
  EXPECT_EQ(structure.Get<DynamicStruct>("ss_header").Convert<uint8_t>("len"), 50);
  EXPECT_FLOAT_EQ(structure.Convert<float>("uint8"), 1.0);
  EXPECT_EQ(structure.Convert<uint8_t>("uint16"), 2);
  EXPECT_EQ(structure.Convert<uint8_t>("uint32"), 3);
  EXPECT_EQ(structure.Convert<uint8_t>("uint64"), 4);
  EXPECT_EQ(structure.Convert<uint8_t>("int8"), 5);
  EXPECT_EQ(structure.Convert<uint8_t>("int16"), 6);
  EXPECT_EQ(structure.Convert<uint8_t>("int32"), 7);
  EXPECT_EQ(structure.Convert<uint8_t>("int64"), 8);
  EXPECT_EQ(structure.Convert<uint8_t>("boolean"), 1);
  EXPECT_EQ(structure.Convert<uint8_t>("float_type"), 10);
  EXPECT_EQ(structure.Convert<uint8_t>("double_type"), 11);
}

TEST(DynamicStruct, GetConvertIf) {
  DescriptorBuilder types = DescriptorBuilder::FromFile(kYamlFile);
  ASSERT_THAT(types.types(), Contains(Key("PrimitiveTest")));

  const TypeDescriptor& primitive_test = *types["PrimitiveTest"];

  DynamicStruct structure(primitive_test);
  structure.Get<uint8_t>("uint8") = 1;

  EXPECT_EQ(structure.GetIf<uint8_t>("uint9"), nullptr);
  EXPECT_EQ(*structure.GetIf<uint8_t>("uint8"), 1);
  EXPECT_EQ(structure.ConvertIf<float>("uint9"), std::nullopt);
  EXPECT_FLOAT_EQ(*structure.ConvertIf<float>("uint8"), 1.0f);
}

TEST(DynamicStruct, Copy) {
  DescriptorBuilder types = DescriptorBuilder::FromFile(kYamlFile);
  ASSERT_THAT(types.types(), Contains(Key("PrimitiveTest")));

  const TypeDescriptor& primitive_test = *types["PrimitiveTest"];

  DynamicStruct structure(primitive_test);
  structure.Get<uint8_t>("uint8") = 1;

  DynamicStruct another_struct = structure;

  EXPECT_EQ(another_struct.Get<uint8_t>("uint8"), 1);
  another_struct.Get<uint8_t>("uint8") = 2;
  EXPECT_EQ(structure.Get<uint8_t>("uint8"), 1);
}

TEST(DynamicArray, ElemAccess) {
  DescriptorBuilder types = DescriptorBuilder::FromFile(kYamlFile);
  ASSERT_THAT(types.types(), Contains(Key("ArrayTest")));

  const TypeDescriptor& array_test = *types["ArrayTest"];

  DynamicStruct structure(array_test);
  DynamicArray& array_1d = structure.Get<DynamicArray>("array_1d");
  DynamicArray& array_2d = structure.Get<DynamicArray>("array_2d");
  DynamicArray& array_3d = structure.Get<DynamicArray>("array_3d");

  ASSERT_EQ(array_1d.size(), 3);
  ASSERT_EQ(array_2d.size(), 2);
  ASSERT_EQ(array_2d.Get<DynamicArray>(1).size(), 3);
  ASSERT_EQ(array_3d.size(), 1);
  ASSERT_EQ(array_3d.Get<DynamicArray>(0).size(), 2);
  ASSERT_EQ(array_3d.Get<DynamicArray>(0).Get<DynamicArray>(1).size(), 3);

  DynamicStruct& array_elem_1 = array_1d.Get<DynamicStruct>(1);
  DynamicStruct& array_elem_2 =
      array_3d.Get<DynamicArray>(0).Get<DynamicArray>(1).Get<DynamicStruct>(1);

  array_elem_1.Get<uint16_t>("field1") = 23;
  array_elem_2.Get<uint16_t>("field1") = 24;

  EXPECT_EQ(array_1d.Get<DynamicStruct>(1).Get<uint16_t>("field1"), 23);
  EXPECT_EQ(array_1d.Get<DynamicStruct>(0).Get<uint16_t>("field1"), 0);
  EXPECT_EQ(array_3d.Get<DynamicArray>(0).Get<DynamicArray>(1).Get<DynamicStruct>(1).Get<uint16_t>(
                "field1"),
            24);
  EXPECT_EQ(array_3d.Get<DynamicArray>(0).Get<DynamicArray>(1).Get<DynamicStruct>(0).Get<uint16_t>(
                "field1"),
            0);
}

TEST(DynamicStruct, UnpackBitfield) {
  DescriptorBuilder types = DescriptorBuilder::FromFile(kYamlFile);
  ASSERT_THAT(types.types(), Contains(Key("Bitfield4BytesTest")));

  const TypeDescriptor& bitfield = *types["Bitfield4BytesTest"];
  DynamicStruct structure(bitfield);

  const uint8_t bytes[] = {
      0x04, 0x03, 0x02, 0x01, 0x02, 0x01, 0x00, 0x01, 0x08, 0xde,
  };

  structure.Unpack(bytes);
  EXPECT_EQ(structure.Get<DynamicStruct>("ss_header").Get<uint32_t>("uid"), 0x04030201);
  EXPECT_EQ(structure.Get<DynamicStruct>("ss_header").Get<uint16_t>("len"), 0x0201);
  EXPECT_EQ(structure.Get<DynamicStruct>("bitfield").Get<uint8_t>("field0"), 6);
  EXPECT_EQ(structure.Get<DynamicStruct>("bitfield").Get<uint8_t>("field1"), 27);
  EXPECT_EQ(structure.Get<DynamicStruct>("bitfield").Get<uint16_t>("field2"), 264);
}

TEST(DynamicStruct, UnpackEnum) {
  DescriptorBuilder types = DescriptorBuilder::FromFile(kYamlFile);
  ASSERT_THAT(types.types(), Contains(Key("Enum2BytesTest")));

  const TypeDescriptor& enumeration = *types["Enum2BytesTest"];
  DynamicStruct structure(enumeration);

  const uint8_t bytes[] = {
      0x04, 0x03, 0x02, 0x01, 0x02, 0x01, 0x00, 0x80,
  };

  structure.Unpack(bytes);
  EXPECT_EQ(structure.Get<DynamicStruct>("ss_header").Get<uint32_t>("uid"), 0x04030201);
  EXPECT_EQ(structure.Get<DynamicStruct>("ss_header").Get<uint16_t>("len"), 0x0201);
  EXPECT_EQ(structure.Get<int16_t>("enumeration"), 128);
}

TEST(DynamicStruct, UnpackPrimitive) {
  DescriptorBuilder types = DescriptorBuilder::FromFile(kYamlFile);
  ASSERT_THAT(types.types(), Contains(Key("PrimitiveTest")));

  const TypeDescriptor& primitive_test = *types["PrimitiveTest"];
  DynamicStruct structure(primitive_test);

  const uint8_t bytes[] = {
      0x04, 0x03, 0x02, 0x01,  // uid
      0x02, 0x01,  // len, 4
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

  structure.Unpack(bytes);
  EXPECT_EQ(structure.Get<DynamicStruct>("ss_header").Get<uint32_t>("uid"), 0x04030201);
  EXPECT_EQ(structure.Get<DynamicStruct>("ss_header").Get<uint16_t>("len"), 0x0201);
  EXPECT_EQ(structure.Get<uint8_t>("uint8"), 0x01);
  EXPECT_EQ(structure.Get<uint16_t>("uint16"), 0x0201);
  EXPECT_EQ(structure.Get<uint32_t>("uint32"), 0x04030201);
  EXPECT_EQ(structure.Get<uint64_t>("uint64"), 0x0807060504030201);
  EXPECT_EQ(structure.Get<int8_t>("int8"), 0x01);
  EXPECT_EQ(structure.Get<int16_t>("int16"), 0x0201);
  EXPECT_EQ(structure.Get<int32_t>("int32"), 0x04030201);
  EXPECT_EQ(structure.Get<int64_t>("int64"), 0x0807060504030201);
  EXPECT_EQ(structure.Get<bool>("boolean"), true);
  EXPECT_FLOAT_EQ(structure.Get<float>("float_type"), 3.1415926f);
  EXPECT_DOUBLE_EQ(structure.Get<double>("double_type"), 3.1415926);
}

TEST(DynamicStruct, UnpackArray) {
  DescriptorBuilder types = DescriptorBuilder::FromFile(kYamlFile);
  ASSERT_THAT(types.types(), Contains(Key("ArrayTest")));

  const TypeDescriptor& array = *types["ArrayTest"];
  DynamicStruct structure(array);

  const uint8_t bytes[] = {
      0x04, 0x03, 0x02, 0x01, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00,
      0x03, 0x00, 0x00, 0x04, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
      0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0x00, 0x05,
  };

  structure.Unpack(bytes);
  EXPECT_EQ(structure.Get<DynamicStruct>("ss_header").Get<uint32_t>("uid"), 0x04030201);
  EXPECT_EQ(structure.Get<DynamicStruct>("ss_header").Get<uint16_t>("len"), 0x0201);

  EXPECT_EQ(structure.Get<DynamicArray>("array_1d").Get<DynamicStruct>(0).Get<uint16_t>("field1"),
            0);
  EXPECT_EQ(structure.Get<DynamicArray>("array_1d").Get<DynamicStruct>(1).Get<uint16_t>("field1"),
            1);
  EXPECT_EQ(structure.Get<DynamicArray>("array_1d").Get<DynamicStruct>(2).Get<uint16_t>("field1"),
            2);

  EXPECT_EQ(structure.Get<DynamicArray>("array_2d")
                .Get<DynamicArray>(0)
                .Get<DynamicStruct>(0)
                .Get<uint16_t>("field1"),
            0);
  EXPECT_EQ(structure.Get<DynamicArray>("array_2d")
                .Get<DynamicArray>(0)
                .Get<DynamicStruct>(1)
                .Get<uint16_t>("field1"),
            1);
  EXPECT_EQ(structure.Get<DynamicArray>("array_2d")
                .Get<DynamicArray>(0)
                .Get<DynamicStruct>(2)
                .Get<uint16_t>("field1"),
            2);
  EXPECT_EQ(structure.Get<DynamicArray>("array_2d")
                .Get<DynamicArray>(1)
                .Get<DynamicStruct>(0)
                .Get<uint16_t>("field1"),
            3);
  EXPECT_EQ(structure.Get<DynamicArray>("array_2d")
                .Get<DynamicArray>(1)
                .Get<DynamicStruct>(1)
                .Get<uint16_t>("field1"),
            4);
  EXPECT_EQ(structure.Get<DynamicArray>("array_2d")
                .Get<DynamicArray>(1)
                .Get<DynamicStruct>(2)
                .Get<uint16_t>("field1"),
            5);

  EXPECT_EQ(structure.Get<DynamicArray>("array_3d")
                .Get<DynamicArray>(0)
                .Get<DynamicArray>(0)
                .Get<DynamicStruct>(0)
                .Get<uint16_t>("field1"),
            0);
  EXPECT_EQ(structure.Get<DynamicArray>("array_3d")
                .Get<DynamicArray>(0)
                .Get<DynamicArray>(0)
                .Get<DynamicStruct>(1)
                .Get<uint16_t>("field1"),
            1);
  EXPECT_EQ(structure.Get<DynamicArray>("array_3d")
                .Get<DynamicArray>(0)
                .Get<DynamicArray>(0)
                .Get<DynamicStruct>(2)
                .Get<uint16_t>("field1"),
            2);
  EXPECT_EQ(structure.Get<DynamicArray>("array_3d")
                .Get<DynamicArray>(0)
                .Get<DynamicArray>(1)
                .Get<DynamicStruct>(0)
                .Get<uint16_t>("field1"),
            3);
  EXPECT_EQ(structure.Get<DynamicArray>("array_3d")
                .Get<DynamicArray>(0)
                .Get<DynamicArray>(1)
                .Get<DynamicStruct>(1)
                .Get<uint16_t>("field1"),
            4);
  EXPECT_EQ(structure.Get<DynamicArray>("array_3d")
                .Get<DynamicArray>(0)
                .Get<DynamicArray>(1)
                .Get<DynamicStruct>(2)
                .Get<uint16_t>("field1"),
            5);
}
