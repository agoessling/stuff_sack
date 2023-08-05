#include "src/crc32.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace ss;

// Test sequence CRCs generated from https://crccalc.com/

TEST(Crc32, CArray) {
  uint8_t input[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
  EXPECT_EQ(GetCrc32(input, sizeof(input)), 0x456CD746);
}

TEST(Crc32, CppArray) {
  {
    std::array<uint8_t, 10> input = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
    EXPECT_EQ(GetCrc32(input), 0x456CD746);
  }
  {
    std::array<uint16_t, 5> input = {0x0000, 0x0101, 0x0202, 0x0303, 0x0404};
    EXPECT_EQ(GetCrc32(input), 0xCDAD819D);
  }
}

TEST(Crc32, Vector) {
  {
    std::vector<uint8_t> input = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
    EXPECT_EQ(GetCrc32(input), 0x456CD746);
  }
  {
    std::vector<uint16_t> input = {0x0000, 0x0101, 0x0202, 0x0303, 0x0404};
    EXPECT_EQ(GetCrc32(input), 0xCDAD819D);
  }
}

TEST(Crc32, String) {
  std::string input = "Hello World!";
  EXPECT_EQ(GetCrc32(input), 0x1C291CA3);
}
