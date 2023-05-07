#include <cstdlib>
#include <iostream>
#include <string>
#include <system_error>
#include <unordered_map>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "src/log_reader.h"
#include "src/log_reader_types.h"

extern "C" {
#include "test/test_message_def.h"
}

using namespace ss;
using namespace testing;

TEST(Open, NonExistent) {
  EXPECT_THROW({ LogReader log_reader("test/nonexistent.ss"); }, std::system_error);
}

TEST(Open, NoDelimiter) {
  EXPECT_THROW({ LogReader log_reader("test/test_log_no_delimiter.ss"); }, LogParseException);
}

TEST(ParseHeader, GetMessages) {
  const char *tmp_dir = getenv("TEST_TMPDIR");
  if (!tmp_dir) {
    FAIL() << "Could not get writable directory from TEST_TMPDIR";
  }

  const std::string path = std::string(tmp_dir) + "/test_log.ss";

  FILE *const file = fopen(path.c_str(), "w+");

  if (!file) {
    FAIL() << strerror(errno);
  }

  ASSERT_GT(SsWriteLogHeader(file), 0);

  Bitfield2BytesTest bitfield_2_test = {.bitfield = {.field1 = 3}};
  Bitfield4BytesTest bitfield_4_test = {.bitfield = {.field2 = 2}};
  Enum1BytesTest enum_1_test = {.enumeration = kEnum1BytesValue3};
  Enum2BytesTest enum_2_test = {.enumeration = kEnum2BytesValue1};
  PrimitiveTest primitive_test = {.int8 = 1};
  ArrayTest array_test = {.array_2d = {{}, {{}, {.field1 = 5}}}};

  for (int i = 0; i < 1000000; ++i) {
    SsLogBitfield2BytesTest(file, &bitfield_2_test);
    bitfield_2_test.bitfield.field1++;
    SsLogBitfield4BytesTest(file, &bitfield_4_test);
    bitfield_4_test.bitfield.field2++;
    SsLogEnum1BytesTest(file, &enum_1_test);
    enum_1_test.enumeration = static_cast<Enum1Bytes>(enum_1_test.enumeration + 1);
    SsLogEnum2BytesTest(file, &enum_2_test);
    enum_2_test.enumeration = static_cast<Enum2Bytes>(enum_2_test.enumeration + 1);
    SsLogPrimitiveTest(file, &primitive_test);
    primitive_test.int8++;
    SsLogArrayTest(file, &array_test);
    array_test.array_2d[0][1].field1++;
  }

  fclose(file);

  LogReader log_reader(path);
  std::unordered_map<std::string, TypeBox> msgs = log_reader.GetMessageTypes();

  std::cout << "Messages:" << std::endl;
  for (const auto& [name, msg] : msgs) {
    std::cout << "  " << name << ": " << msg->PackedSize() << std::endl;
  }

  std::vector<TypeBox *> read_types = {
    &msgs["Bitfield2BytesTest"]["bitfield"]["field1"],
    &msgs["PrimitiveTest"],
  };

  //log_reader.Load(read_types);
  auto types = log_reader.LoadAll();
  (void)types;
}
