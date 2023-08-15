import argparse

import src.stuff_sack as ss
import src.c_stuff_sack as c_ss
from src import utils


def status_enum():
  return '''\
enum class Status {
  kSuccess = 0,
  kInvalidUid = 1,
  kInvalidLen = 2,
};'''


def message_type_enum(messages):
  n = '\n'
  return f'''\
enum class MsgType {{
{f',{n}'.join([f'  k{x.name} = {i}' for i, x in enumerate(messages)])},
  kUnknown = {len(messages)},
}};'''


def any_message_variant(messages):
  n = ',\n'
  return f'''\
using AnyMessage = std::variant<
  std::monostate,
{n.join(['  ' + msg.name for msg in messages])}
>;'''


def inspect_header_definition(messages):
  n = '\n'
  return f'''\
static constexpr uint32_t kMessageUids[{len(messages)}] = {{
{n.join([f'  {m.uid:#010x},' for m in messages])}
}};

static constexpr MsgType kMessageTypes[{len(messages)}] = {{
{n.join([f'  MsgType::k{m.name},' for m in messages])}
}};

static inline constexpr MsgType GetMsgTypeFromUid(uint32_t uid) {{
  for (size_t i = 0; i < {len(messages)}; ++i) {{
    if (uid == kMessageUids[i]) return kMessageTypes[i];
  }}
  return MsgType::kUnknown;
}}

MsgType InspectHeader(const uint8_t *buffer) {{
  SsHeader header;
  SsUnpackSsHeader(buffer, &header);
  return GetMsgTypeFromUid(header.uid);
}}'''


def unpack_message_definition(messages):
  n = '\n'
  s = f'''\
std::pair<AnyMessage, Status> UnpackMessage(const uint8_t *buffer, size_t len) {{
  if (len < kHeaderPackedSize) return {{std::monostate{{}}, Status::kInvalidLen}};

  const MsgType msg_type = InspectHeader(buffer);\n\n'''

  for msg in messages:
    s += f'''\
  if (msg_type == MsgType::k{msg.name}) {{
    if (len != {msg.name}::kPackedSize) return {{std::monostate{{}}, Status::kInvalidLen}};
    return {msg.name}::UnpackNew(buffer);
  }}\n\n'''

  s += '''\
  return {std::monostate{}, Status::kInvalidUid};
}'''

  return s


def message_dispatcher_declaration(messages):
  n = '\n'
  s = '''\
class MessageDispatcher {
 public:
  Status Unpack(const uint8_t *data, size_t len) const;

  template<typename T>
  void AddCallback(std::function<void(const T&)> func) {\n'''

  first = True
  for msg in messages:
    s += f'''\
    {'if' if first else '} else if'} constexpr (std::is_same_v<T, {msg.name}>) {{
      {utils.camel_to_snake(msg.name)}_callbacks_.emplace_back(std::move(func));
'''
    first = False

  s += f'''\
    }} else {{
      static_assert(always_false_v<T>);
    }}
  }}

 private:
  template <typename T>
  static constexpr bool always_false_v = false;

{n.join([f'  std::vector<std::function<void(const {m.name}&)>> {utils.camel_to_snake(m.name)}_callbacks_;'
    for m in messages])}
}};'''

  return s


def message_dispatcher_definition(messages):
  s = '''\
Status MessageDispatcher::Unpack(const uint8_t *data, size_t len) const {
  const auto [msg, status] = UnpackMessage(data, len);
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
}'''

  return s


def declaration(t):
  if isinstance(t, ss.Primitive):
    return None
  if isinstance(t, ss.Bitfield):
    return c_ss.bitfield_declaration(t)
  if isinstance(t, ss.Enum):
    return enum_declaration(t)
  if isinstance(t, ss.Message):
    return message_declaration(t)
  if isinstance(t, ss.Struct):
    return c_ss.struct_declaration(t)

  raise TypeError(f'Unknown type: {type(t)}')


def enum_declaration(enum):
  n = '\n'
  return f'''\
enum class {enum.name} : int{enum.bytes * 8}_t {{
{n.join([f'  k{v.name} = {v.value},' for v in enum.values])}
}};'''


def message_declaration(msg):
  n = '\n'
  return f'''\
struct {msg.name} {{
{n.join([f'  {c_ss.struct_field_declaration(f)};' for f in msg.fields])}

  static constexpr MsgType kType = MsgType::k{msg.name};
  static constexpr uint32_t kUid = {msg.uid:#010x};
  static constexpr size_t kPackedSize = {msg.packed_size};

  static std::pair<{msg.name}, Status> UnpackNew(const uint8_t *buffer);

  void Pack(uint8_t *buffer);
  std::unique_ptr<std::array<uint8_t, kPackedSize>> Pack();
  Status Unpack(const uint8_t *buffer);
}};'''


def message_pack(msg):
  return f'''\
void {msg.name}::Pack(uint8_t *buffer) {{
  ss_header.uid = kUid;
  ss_header.len = kPackedSize;
  {c_ss.pack_function_name(msg)}(this, buffer);
}}

std::unique_ptr<std::array<uint8_t, {msg.name}::kPackedSize>> {msg.name}::Pack() {{
  auto buffer = std::make_unique<std::array<uint8_t, kPackedSize>>();
  Pack(buffer->data());
  return buffer;
}}'''


def message_unpack(msg):
  return f'''\
Status {msg.name}::Unpack(const uint8_t *buffer) {{
  {c_ss.unpack_function_name(msg.fields[0].type)}(buffer + 0, &ss_header);

  if (ss_header.uid != kUid) return Status::kInvalidUid;
  if (ss_header.len != kPackedSize) return Status::kInvalidLen;

  {c_ss.unpack_function_name(msg)}(buffer, this);

  return Status::kSuccess;
}}

std::pair<{msg.name}, Status> {msg.name}::UnpackNew(const uint8_t *buffer) {{
  {msg.name} data;
  const Status status = data.Unpack(buffer);

  if (status != Status::kSuccess) return {{{{}}, status}};

  return {{data, status}};
}}'''


def packing_functions(t):
  if isinstance(t, (ss.Primitive, ss.Bitfield, ss.Enum)):
    return c_ss.primitive_pack(t) + '\n\n' + c_ss.primitive_unpack(t)
  if isinstance(t, ss.Message):
    return c_ss.struct_pack(t) + '\n\n' + c_ss.struct_unpack(t) + '\n\n' + \
        message_pack(t) + '\n\n' + message_unpack(t)
  if isinstance(t, ss.Struct):
    return c_ss.struct_pack(t) + '\n\n' + c_ss.struct_unpack(t)

  raise TypeError('Unknown type: {}'.format(type(t)))


def cc_header(all_types):
  messages = [x for x in all_types if isinstance(x, ss.Message)]

  s = f'''\
#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

namespace ss {{

{status_enum()}

static constexpr size_t kNumMsgTypes = {len(messages)};

{message_type_enum(messages)}

static constexpr size_t kHeaderPackedSize = 6;

MsgType InspectHeader(const uint8_t *buffer);

'''

  for t in all_types:
    d = declaration(t)
    if d:
      s += d + '\n\n'

  s += f'''\
{any_message_variant(messages)}

std::pair<AnyMessage, Status> UnpackMessage(const uint8_t *buffer, size_t len);

{message_dispatcher_declaration(messages)}

}}  // namespace ss\n'''

  return s


def cc_source(all_types, header):
  messages = [x for x in all_types if isinstance(x, ss.Message)]

  s = f'''\
#include "{header}"

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <array>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

namespace ss {{

{c_ss.static_assert(all_types, include_enums=False)}

'''

  for t in all_types:
    s += packing_functions(t) + '\n\n'

  s += f'''\
{inspect_header_definition(messages)}

{unpack_message_definition(messages)}

{message_dispatcher_definition(messages)}

}}  // namespace ss\n'''

  return s


def get_extra_enums(messages):
  msg_type = ss.Enum('MsgType', 'Message types.')
  for m in messages:
    msg_type.add_value(ss.EnumValue(m.name, m.description))
  msg_type.add_value(ss.EnumValue('Unknown', 'Unknown message type.'))

  status = ss.Enum('Status', 'Stuff Sack status code.')
  status.add_value(ss.EnumValue('Success', 'Success.'))
  status.add_value(ss.EnumValue('InvalidUid', 'Invalid message UID.'))
  status.add_value(ss.EnumValue('InvalidLen', 'Invalid message length.'))

  return [msg_type, status]


def main():
  parser = argparse.ArgumentParser(description='Generate message pack C++ library.')
  parser.add_argument('--spec', required=True, help='YAML message specification.')
  parser.add_argument('--header', required=True, help='Library header file name.')
  parser.add_argument('--source', required=True, help='Library source file name.')
  args = parser.parse_args()

  all_types = ss.parse_yaml(args.spec)

  with open(args.header, 'w') as f:
    f.write(cc_header(all_types))

  with open(args.source, 'w') as f:
    f.write(cc_source(all_types, args.header))


if __name__ == '__main__':
  main()
