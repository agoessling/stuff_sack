import argparse

import src.stuff_sack as ss
import src.c_stuff_sack as c_ss
from src import utils


def cc_header(all_types, c_header):
  messages = [x for x in all_types if isinstance(x, ss.Message)]

  s = ''
  s += '#pragma once\n\n'
  s += '#include <cstddef>\n'
  s += '#include <array>\n'
  s += '#include <memory>\n'
  s += '#include <utility>\n'
  s += '#include <variant>\n\n'

  s += 'namespace ss_c {\n'
  s += 'extern "C" {\n'
  s += '#include "{}"\n'.format(c_header)
  s += '}\n} // namespace ss_c\n\n'

  s += 'namespace ss {\n\n'

  s += \
'''enum class Status {
  kSsStatusSuccess = ss_c::kSsStatusSuccess,
  kSsStatusInvalidUid = ss_c::kSsStatusInvalidUid,
  kSsStatusInvalidLen = ss_c::kSsStatusInvalidLen,
};\n\n'''

  for msg in messages:
    s += 'struct {} : public ss_c::{} {{\n'.format(msg.name, msg.name)

    s += _indent('static constexpr size_t kPackedSize = {};\n\n'.format(c_ss.packed_size_name(msg)))

    s += _indent('void Pack(uint8_t *buffer) {\n')
    s += _indent('ss_c::{}(this, buffer);\n'.format(c_ss.pack_function_name(msg)), 2)
    s += _indent('}\n\n')

    s += _indent('std::unique_ptr<std::array<uint8_t, kPackedSize>> Pack() {\n')
    s += _indent('auto buffer = std::make_unique<std::array<uint8_t, kPackedSize>>();\n', 2)
    s += _indent('ss_c::{}(this, buffer->data());\n'.format(c_ss.pack_function_name(msg)), 2)
    s += _indent('return buffer;\n', 2)
    s += _indent('}\n\n')

    s += _indent('Status UnpackInto(const uint8_t *buffer) {\n')
    s += _indent(
        'return static_cast<Status>(ss_c::{}(buffer, this));\n'.format(
            c_ss.unpack_function_name(msg)), 2)
    s += _indent('}\n\n')

    s += _indent('static std::pair<Status, {}> UnpackNew(const uint8_t *buffer) {{\n'.format(
        msg.name))
    s += _indent('{} msg;\n'.format(msg.name), 2)
    s += _indent(
        'Status status = static_cast<Status>(ss_c::{}(buffer, &msg));\n'.format(
            c_ss.unpack_function_name(msg)), 2)
    s += _indent('return {status, msg};\n', 2)
    s += _indent('}\n')

    s += '};\n\n'

  s += 'enum class MsgType {\n'
  for msg in messages:
    s += _indent('k{} = ss_c::kMsgType{},\n'.format(msg.name, msg.name))
  s += '};\n\n'

  s += 'using AnyMessage = std::variant<\n'
  s += _indent('std::monostate,\n', 2)
  for msg in messages:
    s += _indent('{},\n'.format(msg.name), 2)
  s += '>;\n\n'

  s += 'std::pair<Status, AnyMessage> UnpackMessage(const uint8_t *buffer, size_t len) {\n'
  s += _indent('MsgType msg_type = InspectHeader(buffer);\n\n')
  for msg in messages:
    s += _indent('if (msg_type == MsgType::k{}) {{\n'.format(msg.name))
    s += _indent('if (len != {}::kPackedSize) {{\n'.format(msg.name), 2)
    s += _indent('return {Status::kInvalidLen, {}};\n', 3)
    s += _indent('}\n', 2)
    s += _indent('return {}::UnpackNew(buffer);\n'.format(msg.name), 2)
    s += _indent('}\n\n')

  s += _indent('return {Status::kInvalidUid, {}};\n')
  s += '}\n\n'

  s += '} // namespace ss\n'

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
