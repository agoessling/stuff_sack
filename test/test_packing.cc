#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "src/packing.h"

using namespace ss;
using namespace testing;

TEST(Pack, BigEndian) {
  {
    uint8_t buf[1];
    PackBe(int8_t{-15}, buf);
    EXPECT_THAT(buf, ElementsAre(0xF1));
  }
  {
    uint8_t buf[1];
    PackBe(uint8_t{0x54}, buf);
    EXPECT_THAT(buf, ElementsAre(0x54));
  }
  {
    uint8_t buf[1];
    PackBe(bool{true}, buf);
    EXPECT_THAT(buf, ElementsAre(0x01));
  }
  {
    uint8_t buf[2];
    PackBe(int16_t{-559}, buf);
    EXPECT_THAT(buf, ElementsAre(0xFD, 0xD1));
  }
  {
    uint8_t buf[2];
    PackBe(uint16_t{0x5438}, buf);
    EXPECT_THAT(buf, ElementsAre(0x54, 0x38));
  }
  {
    uint8_t buf[4];
    PackBe(int32_t{-559838}, buf);
    EXPECT_THAT(buf, ElementsAre(0xFF, 0xF7, 0x75, 0x22));
  }
  {
    uint8_t buf[4];
    PackBe(uint32_t{0x54382903}, buf);
    EXPECT_THAT(buf, ElementsAre(0x54, 0x38, 0x29, 0x03));
  }
  {
    uint8_t buf[4];
    PackBe(float{3.14159}, buf);
    EXPECT_THAT(buf, ElementsAre(0x40, 0x49, 0x0F, 0xD0));
  }
  {
    uint8_t buf[8];
    PackBe(int64_t{-3829399492848}, buf);
    EXPECT_THAT(buf, ElementsAre(0xFF, 0xFF, 0xFC, 0x84, 0x66, 0x00, 0xE7, 0x10));
  }
  {
    uint8_t buf[8];
    PackBe(uint64_t{0x0123456789ABCDEF}, buf);
    EXPECT_THAT(buf, ElementsAre(0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF));
  }
  {
    uint8_t buf[8];
    PackBe(double{3.14159}, buf);
    EXPECT_THAT(buf, ElementsAre(0x40, 0x09, 0x21, 0xF9, 0xF0, 0x1B, 0x86, 0x6E));
  }
}

TEST(Unpack, BigEndian) {
  {
    uint8_t buf[1] = {0xF1};
    EXPECT_THAT(UnpackBe<int8_t>(buf), int8_t{-15});
  }
  {
    uint8_t buf[1] = {0x54};
    EXPECT_THAT(UnpackBe<uint8_t>(buf), uint8_t{0x54});
  }
  {
    uint8_t buf[1] = {0x01};
    EXPECT_THAT(UnpackBe<bool>(buf), true);
  }
  {
    uint8_t buf[2] = {0xFD, 0xD1};
    EXPECT_THAT(UnpackBe<int16_t>(buf), int16_t{-559});
  }
  {
    uint8_t buf[2] = {0x54, 0x38};
    EXPECT_THAT(UnpackBe<uint16_t>(buf), uint16_t{0x5438});
  }
  {
    uint8_t buf[4] = {0xFF, 0xF7, 0x75, 0x22};
    EXPECT_THAT(UnpackBe<int32_t>(buf), int32_t{-559838});
  }
  {
    uint8_t buf[4] = {0x54, 0x38, 0x29, 0x03};
    EXPECT_THAT(UnpackBe<uint32_t>(buf), uint32_t{0x54382903});
  }
  {
    uint8_t buf[4] = {0x40, 0x49, 0x0F, 0xD0};
    EXPECT_THAT(UnpackBe<float>(buf), float{3.14159});
  }
  {
    uint8_t buf[8] = {0xFF, 0xFF, 0xFC, 0x84, 0x66, 0x00, 0xE7, 0x10};
    EXPECT_THAT(UnpackBe<int64_t>(buf), int64_t{-3829399492848});
  }
  {
    uint8_t buf[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    EXPECT_THAT(UnpackBe<uint64_t>(buf), uint64_t{0x0123456789ABCDEF});
  }
  {
    uint8_t buf[8] = {0x40, 0x09, 0x21, 0xF9, 0xF0, 0x1B, 0x86, 0x6E};
    EXPECT_THAT(UnpackBe<double>(buf), double{3.14159});
  }
}
