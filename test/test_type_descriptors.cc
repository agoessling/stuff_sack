#include <fstream>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "src/type_descriptors.h"

using namespace ss;
using namespace testing;

using Type = TypeDescriptor::Type;
using PrimType = TypeDescriptor::PrimType;

const std::string kYamlFile = "test/test_message_spec.yaml";

static Matcher<TypeDescriptor::FieldList> FieldDescriptorMatcher(
    std::vector<std::pair<std::string, const TypeDescriptor *>> fields) {
  std::vector<Matcher<std::unique_ptr<const FieldDescriptor>>> field_matcher;
  for (const auto& [name, type] : fields) {
    field_matcher.push_back(Pointee(AllOf(Property("name", &FieldDescriptor::name, Eq(name)),
                                          Property("type", &FieldDescriptor::type, Eq(type)))));
  }

  return ElementsAreArray(field_matcher);
}

TEST(Parse, SpecFile) {
  ASSERT_NO_THROW(DescriptorBuilder::FromFile(kYamlFile));
}

TEST(Parse, SpecString) {
  std::ifstream ifs(kYamlFile);
  std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  ASSERT_NO_THROW(DescriptorBuilder::FromString(content));
}

TEST(Parse, BasicTypes) {
  DescriptorBuilder types = DescriptorBuilder::FromString("");

  ASSERT_THAT(types.types(),
              IsSupersetOf({Key("uint8"), Key("uint16"), Key("uint32"), Key("uint64"), Key("int8"),
                            Key("int16"), Key("int32"), Key("int64"), Key("bool"), Key("float"),
                            Key("double")}));

  EXPECT_EQ(types["uint8"]->name(), "uint8");
  EXPECT_EQ(types["uint8"]->type(), Type::kPrimitive);
  EXPECT_EQ(types["uint8"]->packed_size(), 1);
  EXPECT_EQ(types["uint8"]->prim_type(), PrimType::kUint8);

  EXPECT_EQ(types["uint16"]->name(), "uint16");
  EXPECT_EQ(types["uint16"]->type(), Type::kPrimitive);
  EXPECT_EQ(types["uint16"]->packed_size(), 2);
  EXPECT_EQ(types["uint16"]->prim_type(), PrimType::kUint16);

  EXPECT_EQ(types["uint32"]->name(), "uint32");
  EXPECT_EQ(types["uint32"]->type(), Type::kPrimitive);
  EXPECT_EQ(types["uint32"]->packed_size(), 4);
  EXPECT_EQ(types["uint32"]->prim_type(), PrimType::kUint32);

  EXPECT_EQ(types["uint64"]->name(), "uint64");
  EXPECT_EQ(types["uint64"]->type(), Type::kPrimitive);
  EXPECT_EQ(types["uint64"]->packed_size(), 8);
  EXPECT_EQ(types["uint64"]->prim_type(), PrimType::kUint64);

  EXPECT_EQ(types["int8"]->name(), "int8");
  EXPECT_EQ(types["int8"]->type(), Type::kPrimitive);
  EXPECT_EQ(types["int8"]->packed_size(), 1);
  EXPECT_EQ(types["int8"]->prim_type(), PrimType::kInt8);

  EXPECT_EQ(types["int16"]->name(), "int16");
  EXPECT_EQ(types["int16"]->type(), Type::kPrimitive);
  EXPECT_EQ(types["int16"]->packed_size(), 2);
  EXPECT_EQ(types["int16"]->prim_type(), PrimType::kInt16);

  EXPECT_EQ(types["int32"]->name(), "int32");
  EXPECT_EQ(types["int32"]->type(), Type::kPrimitive);
  EXPECT_EQ(types["int32"]->packed_size(), 4);
  EXPECT_EQ(types["int32"]->prim_type(), PrimType::kInt32);

  EXPECT_EQ(types["int64"]->name(), "int64");
  EXPECT_EQ(types["int64"]->type(), Type::kPrimitive);
  EXPECT_EQ(types["int64"]->packed_size(), 8);
  EXPECT_EQ(types["int64"]->prim_type(), PrimType::kInt64);

  EXPECT_EQ(types["bool"]->name(), "bool");
  EXPECT_EQ(types["bool"]->type(), Type::kPrimitive);
  EXPECT_EQ(types["bool"]->packed_size(), 1);
  EXPECT_EQ(types["bool"]->prim_type(), PrimType::kBool);

  EXPECT_EQ(types["float"]->name(), "float");
  EXPECT_EQ(types["float"]->type(), Type::kPrimitive);
  EXPECT_EQ(types["float"]->packed_size(), 4);
  EXPECT_EQ(types["float"]->prim_type(), PrimType::kFloat);

  EXPECT_EQ(types["double"]->name(), "double");
  EXPECT_EQ(types["double"]->type(), Type::kPrimitive);
  EXPECT_EQ(types["double"]->packed_size(), 8);
  EXPECT_EQ(types["double"]->prim_type(), PrimType::kDouble);
}

TEST(Parse, SsHeader) {
  DescriptorBuilder types = DescriptorBuilder::FromString("");

  ASSERT_THAT(types.types(), Contains(Key("SsHeader")));

  const TypeDescriptor *type = types["SsHeader"];
  EXPECT_EQ(type->name(), "SsHeader");
  EXPECT_EQ(type->type(), Type::kStruct);
  EXPECT_EQ(type->packed_size(), 6);

  EXPECT_THAT(type->struct_fields(),
              FieldDescriptorMatcher({{"uid", types["uint32"]}, {"len", types["uint16"]}}));
}

TEST(Parse, Bitfield) {
  DescriptorBuilder types = DescriptorBuilder::FromFile(kYamlFile);

  {
    ASSERT_THAT(types.types(), Contains(Key("Bitfield2Bytes")));

    const TypeDescriptor *type = types["Bitfield2Bytes"];
    EXPECT_EQ(type->name(), "Bitfield2Bytes");
    EXPECT_EQ(type->type(), Type::kBitfield);
    EXPECT_EQ(type->prim_type(), PrimType::kUint16);
    EXPECT_EQ(type->packed_size(), 2);

    EXPECT_THAT(type->struct_fields(), FieldDescriptorMatcher({{"field0", types["uint8"]},
                                                               {"field1", types["uint8"]},
                                                               {"field2", types["uint8"]}}));
  }

  {
    ASSERT_THAT(types.types(), Contains(Key("Bitfield4Bytes")));

    const TypeDescriptor *type = types["Bitfield4Bytes"];
    EXPECT_EQ(type->name(), "Bitfield4Bytes");
    EXPECT_EQ(type->type(), Type::kBitfield);
    EXPECT_EQ(type->prim_type(), PrimType::kUint32);
    EXPECT_EQ(type->packed_size(), 4);

    EXPECT_THAT(type->struct_fields(), FieldDescriptorMatcher({{"field0", types["uint8"]},
                                                               {"field1", types["uint8"]},
                                                               {"field2", types["uint16"]}}));
  }
}

TEST(Parse, Enum) {
  DescriptorBuilder types = DescriptorBuilder::FromFile(kYamlFile);

  {
    ASSERT_THAT(types.types(), Contains(Key("Enum1Bytes")));

    const TypeDescriptor *type = types["Enum1Bytes"];
    EXPECT_EQ(type->name(), "Enum1Bytes");
    EXPECT_EQ(type->type(), Type::kEnum);
    EXPECT_EQ(type->packed_size(), 1);

    std::vector<Matcher<std::string>> value_matcher;
    for (int i = 0; i < 127; ++i) {
      value_matcher.emplace_back(Eq("Value" + std::to_string(i)));
    }

    EXPECT_THAT(type->enum_values(), ElementsAreArray(value_matcher));
  }

  {
    ASSERT_THAT(types.types(), Contains(Key("Enum2Bytes")));

    const TypeDescriptor *type = types["Enum2Bytes"];
    EXPECT_EQ(type->name(), "Enum2Bytes");
    EXPECT_EQ(type->type(), Type::kEnum);
    EXPECT_EQ(type->packed_size(), 2);

    std::vector<Matcher<std::string>> value_matcher;
    for (int i = 0; i < 128; ++i) {
      value_matcher.emplace_back(Eq("Value" + std::to_string(i)));
    }

    EXPECT_THAT(type->enum_values(), ElementsAreArray(value_matcher));
  }
}

TEST(Parse, Struct) {
  DescriptorBuilder types = DescriptorBuilder::FromFile(kYamlFile);

  {
    ASSERT_THAT(types.types(), Contains(Key("Bitfield2BytesTest")));

    const TypeDescriptor *type = types["Bitfield2BytesTest"];
    EXPECT_EQ(type->name(), "Bitfield2BytesTest");
    EXPECT_EQ(type->type(), Type::kStruct);
    EXPECT_EQ(type->packed_size(), 8);

    EXPECT_THAT(type->struct_fields(),
                FieldDescriptorMatcher(
                    {{"ss_header", types["SsHeader"]}, {"bitfield", types["Bitfield2Bytes"]}}));
  }

  {
    ASSERT_THAT(types.types(), Contains(Key("Bitfield4BytesTest")));

    const TypeDescriptor *type = types["Bitfield4BytesTest"];
    EXPECT_EQ(type->name(), "Bitfield4BytesTest");
    EXPECT_EQ(type->type(), Type::kStruct);
    EXPECT_EQ(type->packed_size(), 10);

    EXPECT_THAT(type->struct_fields(),
                FieldDescriptorMatcher(
                    {{"ss_header", types["SsHeader"]}, {"bitfield", types["Bitfield4Bytes"]}}));
  }

  {
    ASSERT_THAT(types.types(), Contains(Key("Enum1BytesTest")));

    const TypeDescriptor *type = types["Enum1BytesTest"];
    EXPECT_EQ(type->name(), "Enum1BytesTest");
    EXPECT_EQ(type->type(), Type::kStruct);
    EXPECT_EQ(type->packed_size(), 7);

    EXPECT_THAT(type->struct_fields(),
                FieldDescriptorMatcher(
                    {{"ss_header", types["SsHeader"]}, {"enumeration", types["Enum1Bytes"]}}));
  }

  {
    ASSERT_THAT(types.types(), Contains(Key("Enum2BytesTest")));

    const TypeDescriptor *type = types["Enum2BytesTest"];
    EXPECT_EQ(type->name(), "Enum2BytesTest");
    EXPECT_EQ(type->type(), Type::kStruct);
    EXPECT_EQ(type->packed_size(), 8);

    EXPECT_THAT(type->struct_fields(),
                FieldDescriptorMatcher(
                    {{"ss_header", types["SsHeader"]}, {"enumeration", types["Enum2Bytes"]}}));
  }

  {
    ASSERT_THAT(types.types(), Contains(Key("PrimitiveTest")));

    const TypeDescriptor *type = types["PrimitiveTest"];
    EXPECT_EQ(type->name(), "PrimitiveTest");
    EXPECT_EQ(type->type(), Type::kStruct);
    EXPECT_EQ(type->packed_size(), 49);

    EXPECT_THAT(type->struct_fields(), FieldDescriptorMatcher({{"ss_header", types["SsHeader"]},
                                                               {"uint8", types["uint8"]},
                                                               {"uint16", types["uint16"]},
                                                               {"uint32", types["uint32"]},
                                                               {"uint64", types["uint64"]},
                                                               {"int8", types["int8"]},
                                                               {"int16", types["int16"]},
                                                               {"int32", types["int32"]},
                                                               {"int64", types["int64"]},
                                                               {"boolean", types["bool"]},
                                                               {"float_type", types["float"]},
                                                               {"double_type", types["double"]}}));
  }

  {
    ASSERT_THAT(types.types(), Contains(Key("ArrayElem")));

    const TypeDescriptor *type = types["ArrayElem"];
    EXPECT_EQ(type->name(), "ArrayElem");
    EXPECT_EQ(type->type(), Type::kStruct);
    EXPECT_EQ(type->packed_size(), 3);

    EXPECT_THAT(type->struct_fields(),
                FieldDescriptorMatcher({{"field0", types["bool"]}, {"field1", types["uint16"]}}));
  }
}

TEST(Parse, Array) {
  DescriptorBuilder types = DescriptorBuilder::FromFile(kYamlFile);

  ASSERT_THAT(types.types(), Contains(Key("ArrayTest")));

  const TypeDescriptor *type = types["ArrayTest"];
  EXPECT_EQ(type->name(), "ArrayTest");
  EXPECT_EQ(type->type(), Type::kStruct);
  EXPECT_EQ(type->packed_size(), 51);

  const TypeDescriptor::FieldList& fields = type->struct_fields();
  ASSERT_EQ(fields.size(), 4);

  EXPECT_EQ(fields[0]->name(), "ss_header");
  EXPECT_EQ(fields[0]->type(), types["SsHeader"]);

  EXPECT_EQ(fields[1]->name(), "array_1d");
  const TypeDescriptor *array_1d_type = fields[1]->type();
  EXPECT_EQ(array_1d_type, types["ArrayElem[3]"]);
  EXPECT_EQ(array_1d_type->name(), "ArrayElem[3]");
  EXPECT_EQ(array_1d_type->type(), Type::kArray);
  EXPECT_EQ(array_1d_type->packed_size(), 9);
  EXPECT_EQ(array_1d_type->array_size(), 3);
  EXPECT_EQ(array_1d_type->array_elem_type(), types["ArrayElem"]);

  EXPECT_EQ(fields[2]->name(), "array_2d");
  const TypeDescriptor *array_2d_type = fields[2]->type();
  EXPECT_EQ(array_2d_type, types["ArrayElem[3][2]"]);
  EXPECT_EQ(array_2d_type->name(), "ArrayElem[3][2]");
  EXPECT_EQ(array_2d_type->type(), Type::kArray);
  EXPECT_EQ(array_2d_type->packed_size(), 18);
  EXPECT_EQ(array_2d_type->array_size(), 2);
  EXPECT_EQ(array_2d_type->array_elem_type(), types["ArrayElem[3]"]);

  EXPECT_EQ(fields[3]->name(), "array_3d");
  const TypeDescriptor *array_3d_type = fields[3]->type();
  EXPECT_EQ(array_3d_type, types["ArrayElem[3][2][1]"]);
  EXPECT_EQ(array_3d_type->name(), "ArrayElem[3][2][1]");
  EXPECT_EQ(array_3d_type->type(), Type::kArray);
  EXPECT_EQ(array_3d_type->packed_size(), 18);
  EXPECT_EQ(array_3d_type->array_size(), 1);
  EXPECT_EQ(array_3d_type->array_elem_type(), types["ArrayElem[3][2]"]);
}

TEST(TypeDescriptor, TypeChecks) {
  DescriptorBuilder types = DescriptorBuilder::FromFile(kYamlFile);

  ASSERT_THAT(types.types(), IsSupersetOf({Key("uint8"), Key("Enum1Bytes"), Key("Bitfield2Bytes"),
                                           Key("ArrayTest")}));

  EXPECT_TRUE(types["uint8"]->IsPrimitive());
  EXPECT_TRUE(types["Enum1Bytes"]->IsEnum());
  EXPECT_TRUE(types["Bitfield2Bytes"]->IsBitfield());
  EXPECT_TRUE(types["ArrayTest"]->IsStruct());
  EXPECT_TRUE(types["ArrayTest"]->struct_fields()[1]->type()->IsArray());
}

TEST(TypeDescriptor, FieldLookup) {
  DescriptorBuilder types = DescriptorBuilder::FromFile(kYamlFile);

  ASSERT_THAT(types.types(), IsSupersetOf({Key("Bitfield4Bytes"), Key("PrimitiveTest"),
                                           Key("uint16"), Key("int64")}));

  EXPECT_EQ((*types["Bitfield4Bytes"])["field2"]->name(), "field2");
  EXPECT_EQ((*types["Bitfield4Bytes"])["field2"]->type(), types["uint16"]);
  EXPECT_EQ((*types["PrimitiveTest"])["int64"]->name(), "int64");
  EXPECT_EQ((*types["PrimitiveTest"])["int64"]->type(), types["int64"]);
}
