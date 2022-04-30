#pragma once

#include <cstddef>
#include <array>
#include <memory>
#include <utility>
#include <variant>

namespace ss_c {
extern "C" {
#include "test/test_message_def.h"
}
} // namesapce ss_c

namespace ss {

enum class Status {
  kSuccess = ss_c::kSsStatusSuccess,
  kInvalidUid = ss_c::kSsStatusInvalidUid,
  kInvalidLen = ss_c::kSsStatusInvalidLen,
};

struct Bitfield2BytesTest : public ss_c::Bitfield2BytesTest {
  static constexpr size_t kPackedSize = SS_BITFIELD2_BYTES_TEST_PACKED_SIZE;

  void Pack(uint8_t *buffer) {
    ss_c::SsPackBitfield2BytesTest(this, buffer);
  }

  std::unique_ptr<std::array<uint8_t, kPackedSize>> Pack() {
    auto buffer = std::make_unique<std::array<uint8_t, kPackedSize>>();
    ss_c::SsPackBitfield2BytesTest(this, buffer->data());
    return buffer;
  }

  Status UnpackInto(const uint8_t *buffer) {
    return static_cast<Status>(ss_c::SsUnpackBitfield2BytesTest(buffer, this));
  }

  static std::pair<Status, Bitfield2BytesTest> UnpackNew(const uint8_t *buffer) {
    Bitfield2BytesTest msg;
    Status status = static_cast<Status>(ss_c::SsUnpackBitfield2BytesTest(buffer, &msg));
    return {status, msg};
  }
};

struct Enum1BytesTest : public ss_c::Enum1BytesTest {
  static constexpr size_t kPackedSize = SS_ENUM1_BYTES_TEST_PACKED_SIZE;

  void Pack(uint8_t *buffer) {
    ss_c::SsPackEnum1BytesTest(this, buffer);
  }

  std::unique_ptr<std::array<uint8_t, kPackedSize>> Pack() {
    auto buffer = std::make_unique<std::array<uint8_t, kPackedSize>>();
    ss_c::SsPackEnum1BytesTest(this, buffer->data());
    return buffer;
  }

  Status UnpackInto(const uint8_t *buffer) {
    return static_cast<Status>(ss_c::SsUnpackEnum1BytesTest(buffer, this));
  }

  static std::pair<Status, Enum1BytesTest> UnpackNew(const uint8_t *buffer) {
    Enum1BytesTest msg;
    Status status = static_cast<Status>(ss_c::SsUnpackEnum1BytesTest(buffer, &msg));
    return {status, msg};
  }
};

enum class MsgType {
  kBitfield2BytesTest = ss_c::kSsMsgTypeBitfield2BytesTest,
  kBitfield4BytesTest = ss_c::kSsMsgTypeBitfield4BytesTest,
  kEnum1BytesTest = ss_c::kSsMsgTypeEnum1BytesTest,
  kEnum2BytesTest = ss_c::kSsMsgTypeEnum2BytesTest,
  kPrimitiveTest = ss_c::kSsMsgTypePrimitiveTest,
  kArrayTest = ss_c::kSsMsgTypeArrayTest,
  kUnknown = ss_c::kSsMsgTypeUnknown,
};

using AnyMessage = std::variant<
    std::monostate,
    Bitfield2BytesTest,
    Enum1BytesTest>;

MsgType InspectHeader(const uint8_t *buffer) {
  return static_cast<MsgType>(ss_c::SsInspectHeader(buffer));
}

std::pair<Status, AnyMessage> UnpackMessage(const uint8_t *buffer) {
  MsgType msg_type = InspectHeader(buffer);

  if (msg_type == MsgType::kBitfield2BytesTest) {
    return Bitfield2BytesTest::UnpackNew(buffer);
  }

  if (msg_type == MsgType::kEnum1BytesTest) {
    return Enum1BytesTest::UnpackNew(buffer);
  }

  return {Status::kInvalidUid, {}};
}

} // namespace ss
