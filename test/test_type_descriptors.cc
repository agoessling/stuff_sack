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

static Matcher<std::vector<std::unique_ptr<FieldDescriptor>>> FieldDescriptorMatcher(
    std::vector<std::pair<std::string, std::shared_ptr<TypeDescriptor>>> fields) {
  std::vector<Matcher<std::unique_ptr<FieldDescriptor>>> field_matcher;
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
  DescriptorBuilder::TypeMap types = DescriptorBuilder::FromString("");

  ASSERT_THAT(types, IsSupersetOf({Key("uint8"), Key("uint16"), Key("uint32"), Key("uint64"),
                                   Key("int8"), Key("int16"), Key("int32"), Key("int64"),
                                   Key("bool"), Key("float"), Key("double")}));

  EXPECT_EQ(types.at("uint8")->name(), "uint8");
  EXPECT_EQ(types.at("uint8")->type(), Type::kPrimitive);
  EXPECT_EQ(types.at("uint8")->packed_size(), 1);
  EXPECT_EQ(types.at("uint8")->prim_type(), PrimType::kUint8);

  EXPECT_EQ(types.at("uint16")->name(), "uint16");
  EXPECT_EQ(types.at("uint16")->type(), Type::kPrimitive);
  EXPECT_EQ(types.at("uint16")->packed_size(), 2);
  EXPECT_EQ(types.at("uint16")->prim_type(), PrimType::kUint16);

  EXPECT_EQ(types.at("uint32")->name(), "uint32");
  EXPECT_EQ(types.at("uint32")->type(), Type::kPrimitive);
  EXPECT_EQ(types.at("uint32")->packed_size(), 4);
  EXPECT_EQ(types.at("uint32")->prim_type(), PrimType::kUint32);

  EXPECT_EQ(types.at("uint64")->name(), "uint64");
  EXPECT_EQ(types.at("uint64")->type(), Type::kPrimitive);
  EXPECT_EQ(types.at("uint64")->packed_size(), 8);
  EXPECT_EQ(types.at("uint64")->prim_type(), PrimType::kUint64);

  EXPECT_EQ(types.at("int8")->name(), "int8");
  EXPECT_EQ(types.at("int8")->type(), Type::kPrimitive);
  EXPECT_EQ(types.at("int8")->packed_size(), 1);
  EXPECT_EQ(types.at("int8")->prim_type(), PrimType::kInt8);

  EXPECT_EQ(types.at("int16")->name(), "int16");
  EXPECT_EQ(types.at("int16")->type(), Type::kPrimitive);
  EXPECT_EQ(types.at("int16")->packed_size(), 2);
  EXPECT_EQ(types.at("int16")->prim_type(), PrimType::kInt16);

  EXPECT_EQ(types.at("int32")->name(), "int32");
  EXPECT_EQ(types.at("int32")->type(), Type::kPrimitive);
  EXPECT_EQ(types.at("int32")->packed_size(), 4);
  EXPECT_EQ(types.at("int32")->prim_type(), PrimType::kInt32);

  EXPECT_EQ(types.at("int64")->name(), "int64");
  EXPECT_EQ(types.at("int64")->type(), Type::kPrimitive);
  EXPECT_EQ(types.at("int64")->packed_size(), 8);
  EXPECT_EQ(types.at("int64")->prim_type(), PrimType::kInt64);

  EXPECT_EQ(types.at("bool")->name(), "bool");
  EXPECT_EQ(types.at("bool")->type(), Type::kPrimitive);
  EXPECT_EQ(types.at("bool")->packed_size(), 1);
  EXPECT_EQ(types.at("bool")->prim_type(), PrimType::kBool);

  EXPECT_EQ(types.at("float")->name(), "float");
  EXPECT_EQ(types.at("float")->type(), Type::kPrimitive);
  EXPECT_EQ(types.at("float")->packed_size(), 4);
  EXPECT_EQ(types.at("float")->prim_type(), PrimType::kFloat);

  EXPECT_EQ(types.at("double")->name(), "double");
  EXPECT_EQ(types.at("double")->type(), Type::kPrimitive);
  EXPECT_EQ(types.at("double")->packed_size(), 8);
  EXPECT_EQ(types.at("double")->prim_type(), PrimType::kDouble);
}

TEST(Parse, SsHeader) {
  DescriptorBuilder::TypeMap types = DescriptorBuilder::FromString("");

  ASSERT_THAT(types, Contains(Key("SsHeader")));

  std::shared_ptr<TypeDescriptor> type = types.at("SsHeader");
  EXPECT_EQ(type->name(), "SsHeader");
  EXPECT_EQ(type->type(), Type::kStruct);
  EXPECT_EQ(type->packed_size(), 6);

  EXPECT_THAT(type->struct_fields(),
              FieldDescriptorMatcher({{"uid", types.at("uint32")}, {"len", types.at("uint16")}}));
}

TEST(Parse, Bitfield) {
  DescriptorBuilder::TypeMap types = DescriptorBuilder::FromFile(kYamlFile);

  {
    ASSERT_THAT(types, Contains(Key("Bitfield2Bytes")));

    std::shared_ptr<TypeDescriptor> type = types.at("Bitfield2Bytes");
    EXPECT_EQ(type->name(), "Bitfield2Bytes");
    EXPECT_EQ(type->type(), Type::kStruct);
    EXPECT_EQ(type->packed_size(), 2);

    EXPECT_THAT(type->struct_fields(), FieldDescriptorMatcher({{"field0", types.at("uint8")},
                                                               {"field1", types.at("uint8")},
                                                               {"field2", types.at("uint8")}}));
  }

  {
    ASSERT_THAT(types, Contains(Key("Bitfield4Bytes")));

    std::shared_ptr<TypeDescriptor> type = types.at("Bitfield4Bytes");
    EXPECT_EQ(type->name(), "Bitfield4Bytes");
    EXPECT_EQ(type->type(), Type::kStruct);
    EXPECT_EQ(type->packed_size(), 4);

    EXPECT_THAT(type->struct_fields(), FieldDescriptorMatcher({{"field0", types.at("uint8")},
                                                               {"field1", types.at("uint8")},
                                                               {"field2", types.at("uint16")}}));
  }
}

TEST(Parse, Enum) {
  DescriptorBuilder::TypeMap types = DescriptorBuilder::FromFile(kYamlFile);

  {
    ASSERT_THAT(types, Contains(Key("Enum1Bytes")));

    std::shared_ptr<TypeDescriptor> type = types.at("Enum1Bytes");
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
    ASSERT_THAT(types, Contains(Key("Enum2Bytes")));

    std::shared_ptr<TypeDescriptor> type = types.at("Enum2Bytes");
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
  DescriptorBuilder::TypeMap types = DescriptorBuilder::FromFile(kYamlFile);

  {
    ASSERT_THAT(types, Contains(Key("Bitfield2BytesTest")));

    std::shared_ptr<TypeDescriptor> type = types.at("Bitfield2BytesTest");
    EXPECT_EQ(type->name(), "Bitfield2BytesTest");
    EXPECT_EQ(type->type(), Type::kStruct);
    EXPECT_EQ(type->packed_size(), 8);

    EXPECT_THAT(type->struct_fields(),
                FieldDescriptorMatcher({{"ss_header", types.at("SsHeader")},
                                        {"bitfield", types.at("Bitfield2Bytes")}}));
  }

  {
    ASSERT_THAT(types, Contains(Key("Bitfield4BytesTest")));

    std::shared_ptr<TypeDescriptor> type = types.at("Bitfield4BytesTest");
    EXPECT_EQ(type->name(), "Bitfield4BytesTest");
    EXPECT_EQ(type->type(), Type::kStruct);
    EXPECT_EQ(type->packed_size(), 10);

    EXPECT_THAT(type->struct_fields(),
                FieldDescriptorMatcher({{"ss_header", types.at("SsHeader")},
                                        {"bitfield", types.at("Bitfield4Bytes")}}));
  }

  {
    ASSERT_THAT(types, Contains(Key("Enum1BytesTest")));

    std::shared_ptr<TypeDescriptor> type = types.at("Enum1BytesTest");
    EXPECT_EQ(type->name(), "Enum1BytesTest");
    EXPECT_EQ(type->type(), Type::kStruct);
    EXPECT_EQ(type->packed_size(), 7);

    EXPECT_THAT(type->struct_fields(),
                FieldDescriptorMatcher({{"ss_header", types.at("SsHeader")},
                                        {"enumeration", types.at("Enum1Bytes")}}));
  }

  {
    ASSERT_THAT(types, Contains(Key("Enum2BytesTest")));

    std::shared_ptr<TypeDescriptor> type = types.at("Enum2BytesTest");
    EXPECT_EQ(type->name(), "Enum2BytesTest");
    EXPECT_EQ(type->type(), Type::kStruct);
    EXPECT_EQ(type->packed_size(), 8);

    EXPECT_THAT(type->struct_fields(),
                FieldDescriptorMatcher({{"ss_header", types.at("SsHeader")},
                                        {"enumeration", types.at("Enum2Bytes")}}));
  }

  {
    ASSERT_THAT(types, Contains(Key("PrimitiveTest")));

    std::shared_ptr<TypeDescriptor> type = types.at("PrimitiveTest");
    EXPECT_EQ(type->name(), "PrimitiveTest");
    EXPECT_EQ(type->type(), Type::kStruct);
    EXPECT_EQ(type->packed_size(), 49);

    EXPECT_THAT(type->struct_fields(),
                FieldDescriptorMatcher({{"ss_header", types.at("SsHeader")},
                                        {"uint8", types.at("uint8")},
                                        {"uint16", types.at("uint16")},
                                        {"uint32", types.at("uint32")},
                                        {"uint64", types.at("uint64")},
                                        {"int8", types.at("int8")},
                                        {"int16", types.at("int16")},
                                        {"int32", types.at("int32")},
                                        {"int64", types.at("int64")},
                                        {"boolean", types.at("bool")},
                                        {"float_type", types.at("float")},
                                        {"double_type", types.at("double")}}));
  }

  {
    ASSERT_THAT(types, Contains(Key("ArrayElem")));

    std::shared_ptr<TypeDescriptor> type = types.at("ArrayElem");
    EXPECT_EQ(type->name(), "ArrayElem");
    EXPECT_EQ(type->type(), Type::kStruct);
    EXPECT_EQ(type->packed_size(), 3);

    EXPECT_THAT(type->struct_fields(), FieldDescriptorMatcher({{"field0", types.at("bool")},
                                                               {"field1", types.at("uint16")}}));
  }
}

TEST(Parse, Array) {
  DescriptorBuilder::TypeMap types = DescriptorBuilder::FromFile(kYamlFile);

  ASSERT_THAT(types, Contains(Key("ArrayTest")));

  std::shared_ptr<TypeDescriptor> type = types.at("ArrayTest");
  EXPECT_EQ(type->name(), "ArrayTest");
  EXPECT_EQ(type->type(), Type::kStruct);
  EXPECT_EQ(type->packed_size(), 51);

  const std::vector<std::unique_ptr<FieldDescriptor>>& fields = type->struct_fields();
  ASSERT_EQ(fields.size(), 4);

  EXPECT_EQ(fields[0]->name(), "ss_header");
  EXPECT_EQ(fields[0]->type(), types.at("SsHeader"));

  EXPECT_EQ(fields[1]->name(), "array_1d");
  std::shared_ptr<TypeDescriptor> array_1d_type = fields[1]->type();
  EXPECT_EQ(array_1d_type, types.at("ArrayElem[3]"));
  EXPECT_EQ(array_1d_type->name(), "ArrayElem[3]");
  EXPECT_EQ(array_1d_type->type(), Type::kArray);
  EXPECT_EQ(array_1d_type->packed_size(), 9);
  EXPECT_EQ(array_1d_type->array_size(), 3);
  EXPECT_EQ(array_1d_type->array_elem_type(), types.at("ArrayElem"));

  EXPECT_EQ(fields[2]->name(), "array_2d");
  std::shared_ptr<TypeDescriptor> array_2d_type = fields[2]->type();
  EXPECT_EQ(array_2d_type, types.at("ArrayElem[3][2]"));
  EXPECT_EQ(array_2d_type->name(), "ArrayElem[3][2]");
  EXPECT_EQ(array_2d_type->type(), Type::kArray);
  EXPECT_EQ(array_2d_type->packed_size(), 18);
  EXPECT_EQ(array_2d_type->array_size(), 2);
  EXPECT_EQ(array_2d_type->array_elem_type(), types.at("ArrayElem[3]"));

  EXPECT_EQ(fields[3]->name(), "array_3d");
  std::shared_ptr<TypeDescriptor> array_3d_type = fields[3]->type();
  EXPECT_EQ(array_3d_type, types.at("ArrayElem[3][2][1]"));
  EXPECT_EQ(array_3d_type->name(), "ArrayElem[3][2][1]");
  EXPECT_EQ(array_3d_type->type(), Type::kArray);
  EXPECT_EQ(array_3d_type->packed_size(), 18);
  EXPECT_EQ(array_3d_type->array_size(), 1);
  EXPECT_EQ(array_3d_type->array_elem_type(), types.at("ArrayElem[3][2]"));
}

TEST(TypeDescriptor, TypeChecks) {
  DescriptorBuilder::TypeMap types = DescriptorBuilder::FromFile(kYamlFile);

  ASSERT_THAT(types, IsSupersetOf({Key("uint8"), Key("Enum1Bytes"), Key("Bitfield2Bytes"),
                                   Key("ArrayTest")}));

  EXPECT_TRUE(types.at("uint8")->IsPrimitive());
  EXPECT_TRUE(types.at("Enum1Bytes")->IsEnum());
  EXPECT_TRUE(types.at("Bitfield2Bytes")->IsStruct());
  EXPECT_TRUE(types.at("ArrayTest")->IsStruct());
  EXPECT_TRUE(types.at("ArrayTest")->struct_fields()[1]->type()->IsArray());
}

TEST(TypeDescriptor, FieldLookup) {
  DescriptorBuilder::TypeMap types = DescriptorBuilder::FromFile(kYamlFile);

  ASSERT_THAT(types, IsSupersetOf({Key("PrimitiveTest"), Key("int64")}));

  EXPECT_EQ(types.at("PrimitiveTest")->struct_get_field("int64"), types.at("int64"));
}
