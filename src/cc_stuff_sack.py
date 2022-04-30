import argparse

import src.stuff_sack as ss
import src.c_stuff_sack as c_ss
from src import utils


def cc_header(all_types, c_header):
  messages = [x for x in all_types if isinstance(x, ss.Message)]
  structs = [x for x in all_types if isinstance(x, ss.Struct) and not isinstance(x, ss.Message)]
  bitfields = [x for x in all_types if isinstance(x, ss.Bitfield)]
  enums = [x for x in all_types if isinstance(x, ss.Enum)]

  s = f'''\
#pragma once

#include <cstddef>
#include <array>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace ss_c {{
extern "C" {{
#include "{c_header}"
}}
}} // namespace ss_c

namespace ss {{

enum class Status {{
  kSuccess = ss_c::kSsStatusSuccess,
  kInvalidUid = ss_c::kSsStatusInvalidUid,
  kInvalidLen = ss_c::kSsStatusInvalidLen,
}};\n\n'''

  s += '\n'.join([f'using {x.name} = ss_c::{x.name};' for x in enums + bitfields + structs])
  s += '\n\n'

  for msg in messages:
    s += f'''\
struct {msg.name} : public ss_c::{msg.name} {{
  static constexpr size_t kPackedSize = {c_ss.packed_size_name(msg)};

  void Pack(uint8_t *buffer) {{
    ss_c::{c_ss.pack_function_name(msg)}(this, buffer);
  }}

  std::unique_ptr<std::array<uint8_t, kPackedSize>> Pack() {{
    auto buffer = std::make_unique<std::array<uint8_t, kPackedSize>>();
    ss_c::{c_ss.pack_function_name(msg)}(this, buffer->data());
    return buffer;
  }}

  Status UnpackInto(const uint8_t *buffer) {{
    return static_cast<Status>(ss_c::{c_ss.unpack_function_name(msg)}(buffer, this));
  }}

  static std::pair<Status, {msg.name}> UnpackNew(const uint8_t *buffer) {{
    {msg.name} msg;
    Status status = static_cast<Status>(ss_c::{c_ss.unpack_function_name(msg)}(buffer, &msg));
    return {{status, msg}};
  }}
}};\n\n'''

  s += f'static constexpr size_t kNumMsgTypes = {len(messages)};\n\n'

  s += 'enum class MsgType {\n'
  for msg in messages:
    s += f'  k{msg.name} = ss_c::kSsMsgType{msg.name},\n'
  s += '  kUnknown = ss_c::kSsMsgTypeUnknown\n'
  s += '};\n\n'

  s += 'static constexpr size_t kHeaderPackedSize = SS_HEADER_PACKED_SIZE;\n\n'

  s += '''\
static inline MsgType InspectHeader(const uint8_t *buffer) {
  return static_cast<MsgType>(ss_c::SsInspectHeader(buffer));
}\n\n'''

  s += 'using AnyMessage = std::variant<\n'
  s += '  std::monostate,\n'
  s += ',\n'.join(['  ' + msg.name for msg in messages])
  s += '\n>;\n\n'

  s += f'''\
static inline std::pair<Status, AnyMessage> UnpackMessage(const uint8_t *buffer, size_t len) {{
  if (len < kHeaderPackedSize) {{
    return {{Status::kInvalidLen, {{}}}};
  }}

  const MsgType msg_type = static_cast<MsgType>(ss_c::SsInspectHeader(buffer));\n\n'''
  for msg in messages:
    s += f'''\
  if (msg_type == MsgType::k{msg.name}) {{
    if (len != {msg.name}::kPackedSize) {{
      return {{Status::kInvalidLen, {{}}}};
    }}
    return {msg.name}::UnpackNew(buffer);
  }}\n\n'''

  s += '''\
  return {Status::kInvalidUid, {}};
}\n\n'''

  s += '''\
class MessageDispatcher {
 public:
  template<typename T>
  void AddCallback(std::function<void(const T&)> func) {\n'''

  first = True
  for msg in messages:
    s += f'''\
    {'if' if first else '} else if'} constexpr (std::is_same_v<T, {msg.name}>) {{
      {utils.camel_to_snake(msg.name)}_callbacks_.emplace_back(std::move(func));
'''
    first = False

  s += '''\
    } else {
      static_assert(always_false_v<T>);
    }
  }\n\n'''

  s += '''\
  Status Unpack(const uint8_t *data, size_t len) {
    const auto [status, msg] = UnpackMessage(data, len);
    if (status != Status::kSuccess) return status;

    std::visit([this] (auto&& msg) {
      using T  = std::decay_t<decltype(msg)>;\n\n'''

  first = True
  for msg in messages:
    s += f'''\
      {'if' if first else '} else if'} constexpr (std::is_same_v<T, {msg.name}>) {{
        for (const auto& func : {utils.camel_to_snake(msg.name)}_callbacks_) {{
          func(msg);
        }}\n'''
    first = False

  s += '''\
      }
    }, msg);

    return status;
  }\n\n'''

  s += '''\
 private:
  template <typename T>
  static constexpr bool always_false_v = false;\n\n'''

  s += '\n'.join([
      f'  std::vector<std::function<void(const {x.name}&)>> {utils.camel_to_snake(x.name)}_callbacks_;'
      for x in messages
  ])

  s += '\n};\n\n'

  s += '}  // namespace ss\n'

  return s


def main():
  parser = argparse.ArgumentParser(description='Generate message pack C++ library.')
  parser.add_argument('--spec', required=True, help='YAML message specification.')
  parser.add_argument('--header', required=True, help='Library header file name.')
  parser.add_argument('--c_header', required=True, help='C header file name.')
  args = parser.parse_args()

  all_types = ss.parse_yaml(args.spec)

  with open(args.header, 'w') as f:
    f.write(cc_header(all_types, args.c_header))


if __name__ == '__main__':
  main()
