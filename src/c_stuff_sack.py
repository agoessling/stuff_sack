import argparse
import textwrap
import yaml

import src.stuff_sack as ss
from src import utils


def c_type_name(type_object):
  type_mapping = {
      'uint8': 'uint8_t',
      'uint16': 'uint16_t',
      'uint32': 'uint32_t',
      'uint64': 'uint64_t',
      'int8': 'int8_t',
      'int16': 'int16_t',
      'int32': 'int32_t',
      'int64': 'int64_t',
  }

  root_type = type_object.root_type

  if root_type.name in type_mapping:
    return type_mapping[root_type.name]

  return root_type.name


def c_field_name(field):
  length_list = []
  current_type = field.type
  while isinstance(current_type, ss.Array):
    length_list.append(current_type.length)
    current_type = current_type.type

  brackets = ''.join('[{}]'.format(x) for x in length_list)

  return '{}{}'.format(field.name, brackets)


def pack_function_name(obj):
  return 'SsPack{}'.format(utils.snake_to_camel(obj.name))


def unpack_function_name(obj):
  return 'SsUnpack{}'.format(utils.snake_to_camel(obj.name))


def declaration(type_object):
  if type(type_object) is ss.Primitive:
    return None

  if isinstance(type_object, ss.Bitfield):
    return bitfield_declaration(type_object)

  if isinstance(type_object, ss.Enum):
    return enum_declaration(type_object)

  if isinstance(type_object, ss.Struct):
    return struct_declaration(type_object)

  raise TypeError('Unknown type: {}'.format(type(type_object)))


def bitfield_c_type(bitfield):
  return 'uint{}_t'.format(bitfield.bytes * 8)


def bitfield_field_declaration(bitfield, field):
  return '{} {} : {}'.format(bitfield_c_type(bitfield), field.name, field.bits)


def bitfield_declaration(bitfield):
  s = ''
  s += 'typedef struct {\n'

  for field in bitfield.fields:
    s += utils.indent('{};\n'.format(bitfield_field_declaration(bitfield, field)))

  s += '}} {};'.format(bitfield.name)

  return s


def enum_declaration(enum):
  s = ''
  s += 'typedef enum {\n'
  s += utils.indent('k{}ForceSigned = -1,\n'.format(enum.name))

  for value in enum.values:
    s += utils.indent('k{}{} = {},\n'.format(enum.name, value.name, value.value))

  s += utils.indent('kNum{} = {},\n'.format(enum.name, len(enum.values)))
  s += '}} {};'.format(enum.name)

  return s


def struct_field_declaration(field):
  return '{} {}'.format(c_type_name(field.type), c_field_name(field))


def struct_declaration(struct):
  s = ''
  s += 'typedef struct {\n'

  for field in struct.fields:
    s += utils.indent('{};\n'.format(struct_field_declaration(field)))

  s += '}} {};'.format(struct.name)

  return s


def pack(type_object):
  if isinstance(type_object, ss.Bitfield):
    return primitive_union_pack(type_object)

  if type_object.name in ('float', 'double'):
    return primitive_union_pack(type_object)

  if isinstance(type_object, ss.Enum):
    return primitive_pack(type_object)

  if isinstance(type_object, ss.Message):
    return message_pack(type_object)

  if isinstance(type_object, ss.Struct):
    return struct_pack(type_object)

  if isinstance(type_object, ss.Primitive):
    return primitive_pack(type_object)

  raise TypeError('Unknown type: {}'.format(type(type_object)))


def unpack(type_object):
  if isinstance(type_object, ss.Bitfield):
    return primitive_union_unpack(type_object)

  if type_object.name in ('float', 'double'):
    return primitive_union_unpack(type_object)

  if isinstance(type_object, ss.Enum):
    return primitive_unpack(type_object)

  if isinstance(type_object, ss.Message):
    return message_unpack(type_object)

  if isinstance(type_object, ss.Struct):
    return struct_unpack(type_object)

  if isinstance(type_object, ss.Primitive):
    return primitive_unpack(type_object)

  raise TypeError('Unknown type: {}'.format(type(type_object)))


def primitive_pack(obj):
  bits = obj.bytes * 8
  s = ''
  s += 'static inline void {}(const {} *data, uint8_t *buffer) {{\n'.format(
      pack_function_name(obj), c_type_name(obj))
  s += utils.indent('uint{}_t raw_data = (uint{}_t)*data;\n'.format(bits, bits))

  for i in range(obj.bytes):
    s += utils.indent('buffer[{}] = (uint8_t)(raw_data >> {});\n'.format(
        i, (obj.bytes - i - 1) * 8))

  s += '}'

  return s


def primitive_union_pack(obj):
  bits = obj.bytes * 8
  s = ''
  s += 'static inline void {}(const {} *data, uint8_t *buffer) {{\n'.format(
      pack_function_name(obj), c_type_name(obj))
  s += utils.indent('union {\n')
  s += utils.indent('{} data;\n'.format(c_type_name(obj)), 2)
  s += utils.indent('uint{}_t raw_data;\n'.format(bits), 2)
  s += utils.indent('} data_union;\n\n')

  s += utils.indent('data_union.data = *data;\n')

  for i in range(obj.bytes):
    s += utils.indent('buffer[{}] = (uint8_t)(data_union.raw_data >> {});\n'.format(
        i, (obj.bytes - i - 1) * 8))

  s += '}'

  return s


def array_pack(name, obj, offset, index_str='', iter_var='i'):
  if ord(iter_var) > ord('z'):
    raise ValueError('Invalid iteration variable: {}'.format(iter_var))

  offset_str = '{} * {} + {}'.format(iter_var, obj.type.packed_size, offset)
  index_str += '[{}]'.format(iter_var)

  s = ''
  s += 'for(int32_t {0} = 0; {0} < {1}; ++{0}) {{\n'.format(iter_var, obj.length)

  if isinstance(obj.type, ss.Array):
    s += utils.indent(array_pack(name, obj.type, offset_str, index_str, chr(ord(iter_var) + 1)))
  else:
    s += utils.indent('{}(&data->{}{}, buffer + {});\n'.format(pack_function_name(obj.root_type),
                                                               name, index_str, offset_str))

  s += '}\n'

  return s


def message_pack_prototype(obj):
  return 'void {}({} *data, uint8_t *buffer)'.format(pack_function_name(obj), c_type_name(obj))


def message_unpack_prototype(obj):
  return 'SsStatus {}(const uint8_t *buffer, {} *data)'.format(unpack_function_name(obj),
                                                               c_type_name(obj))


def message_pack(obj):
  s = ''
  s += '{} {{\n'.format(message_pack_prototype(obj))

  s += utils.indent('data->ss_header.uid = {:#010x};\n'.format(obj.uid))
  s += utils.indent('data->ss_header.len = {};\n\n'.format(obj.packed_size))

  s += utils.indent('{}\n'.format(struct_pack_body(obj)))

  s += '}'

  return s


def message_unpack(obj):
  s = ''
  s += '{} {{\n'.format(message_unpack_prototype(obj))

  s += utils.indent('{}(buffer + 0, &data->ss_header);\n\n'.format(
      unpack_function_name(obj.fields[0].type)))

  s += utils.indent('if (data->ss_header.uid != {:#010x}) {{\n'.format(obj.uid))
  s += utils.indent('return kSsStatusInvalidUid;\n', 2)
  s += utils.indent('}\n\n')

  s += utils.indent('if (data->ss_header.len != {}) {{\n'.format(obj.packed_size))
  s += utils.indent('return kSsStatusInvalidLen;\n', 2)
  s += utils.indent('}\n\n')

  s += utils.indent('{}\n\n'.format(struct_unpack_body(obj, skip_header=True)))

  s += utils.indent('return kSsStatusSuccess;\n')
  s += '}'

  return s


def struct_pack(obj):
  s = ''
  s += 'static inline void {}(const {} *data, uint8_t *buffer) {{\n'.format(
      pack_function_name(obj), c_type_name(obj))

  s += utils.indent(struct_pack_body(obj))

  s += '}'

  return s


def struct_pack_body(obj):
  s = ''

  offset = 0
  for field in obj.fields:
    if isinstance(field.type, ss.Array):
      s += array_pack(field.name, field.type, offset)
    else:
      s += '{}(&data->{}, buffer + {});\n'.format(pack_function_name(field.type), field.name,
                                                  offset)

    offset += field.type.packed_size

  return s[:-1]


def primitive_unpack(obj):
  bits = obj.bytes * 8
  s = ''
  s += 'static inline void {}(const uint8_t *buffer, {} *data) {{\n'.format(
      unpack_function_name(obj), c_type_name(obj))
  s += utils.indent('uint{}_t raw_data = 0;\n'.format(bits))

  for i in range(obj.bytes):
    s += utils.indent('raw_data |= (uint{}_t)buffer[{}] << {};\n'.format(
        bits, i, (obj.bytes - i - 1) * 8))

  s += utils.indent('*data = ({})raw_data;\n'.format(c_type_name(obj)))
  s += '}'

  return s


def primitive_union_unpack(obj):
  bits = obj.bytes * 8
  s = ''
  s += 'static inline void {}(const uint8_t *buffer, {} *data) {{\n'.format(
      unpack_function_name(obj), c_type_name(obj))

  s += utils.indent('union {\n')
  s += utils.indent('{} data;\n'.format(c_type_name(obj)), 2)
  s += utils.indent('uint{}_t raw_data;\n'.format(bits), 2)
  s += utils.indent('} data_union;\n\n')

  s += utils.indent('data_union.raw_data = 0;\n')

  for i in range(obj.bytes):
    s += utils.indent('data_union.raw_data |= (uint{}_t)buffer[{}] << {};\n'.format(
        bits, i, (obj.bytes - i - 1) * 8))

  s += utils.indent('\n*data = data_union.data;\n')
  s += '}'

  return s


def array_unpack(name, obj, offset, index_str='', iter_var='i'):
  if ord(iter_var) > ord('z'):
    raise ValueError('Invalid iteration variable: {}'.format(iter_var))

  offset_str = '{} * {} + {}'.format(iter_var, obj.type.packed_size, offset)
  index_str += '[{}]'.format(iter_var)

  s = ''
  s += 'for(int32_t {0} = 0; {0} < {1}; ++{0}) {{\n'.format(iter_var, obj.length)

  if isinstance(obj.type, ss.Array):
    s += utils.indent(array_unpack(name, obj.type, offset_str, index_str, chr(ord(iter_var) + 1)))
  else:
    s += utils.indent('{}(buffer + {}, &data->{}{});\n'.format(unpack_function_name(obj.root_type),
                                                               offset_str, name, index_str))

  s += '}\n'

  return s


def struct_unpack(obj):
  s = ''
  s += 'static inline void {}(const uint8_t *buffer, {} *data) {{\n'.format(
      unpack_function_name(obj), c_type_name(obj))

  s += utils.indent('{}\n'.format(struct_unpack_body(obj)))

  s += '}'

  return s


def struct_unpack_body(obj, skip_header=False):
  s = ''

  offset = 0
  for field in obj.fields:
    if skip_header and field.type.name == 'SsHeader':
      pass
    elif isinstance(field.type, ss.Array):
      s += array_unpack(field.name, field.type, offset)
    else:
      s += '{}(buffer + {}, &data->{});\n'.format(unpack_function_name(field.type), offset,
                                                  field.name)

    offset += field.type.packed_size

  return s[:-1]


def packed_size_name(message):
  return 'SS_{}_PACKED_SIZE'.format(utils.camel_to_snake(message.name).upper())


def static_assert(all_types):
  s = ''

  for t in all_types:
    if isinstance(t, (ss.Primitive, ss.Bitfield)):
      s += 'static_assert(sizeof({}) == {}, "{} size mismatch.");\n'.format(
          c_type_name(t), t.bytes, c_type_name(t))
    if isinstance(t, ss.Enum):
      s += 'static_assert(kNum{} < {} / 2, "{} size mismatch.");\n'.format(
          t.name, ' * '.join(['256'] * t.bytes), t.name)

  return s[:-1]


def yaml_log_header(spec, messages):
  spec['SsMessageUidMap'] = {msg.uid: msg.name for msg in messages}
  return f'static const char kYamlHeader[] = "{yaml.dump(spec).encode("unicode_escape").decode()}";'


def write_log_header():
  return '''\
int SsWriteLogHeader(void *fd) {
  int yaml_ret = SsWriteFile(fd, kYamlHeader, sizeof(kYamlHeader) - 1);
  if (yaml_ret < 0) return yaml_ret;

  int delim_ret = SsWriteFile(fd, kSsLogDelimiter, sizeof(kSsLogDelimiter) - 1);
  if (delim_ret < 0) return delim_ret;

  return yaml_ret + delim_ret;
}'''


def find_log_delimiter():
  return '''\
int SsFindLogDelimiter(void *fd) {
  const int kDelimLen = sizeof(kSsLogDelimiter) - 1;
  uint8_t buf[4 * kDelimLen];

  int file_index = 0;
  int delim_index = 0;
  while (true) {
    const int ret = SsReadFile(fd, buf, sizeof(buf));
    if (ret < 0) return ret;

    for (int i = 0; i < ret; ++i) {
      if (buf[i] != kSsLogDelimiter[delim_index]) {
        delim_index = 0;
      }

      if (buf[i] == kSsLogDelimiter[delim_index]) {
        if (++delim_index >= kDelimLen) {
          return file_index + 1;
        }
      }

      file_index++;
    }

    if (ret != sizeof(buf)) return -1;
  }
}'''


def log_function_name(obj):
  return f'SsLog{obj.name}'


def message_log_prototype(obj):
  return f'int {log_function_name(obj)}(void *fd, {obj.name} *data)'


def message_log(obj):
  return f'''\
{message_log_prototype(obj)} {{
  uint8_t buf[{packed_size_name(obj)}];
  {pack_function_name(obj)}(data, buf);
  return SsWriteFile(fd, buf, sizeof(buf));
}}'''


def c_header(all_types):
  messages = [x for x in all_types if isinstance(x, ss.Message)]

  s = ''
  s += '#pragma once\n\n'
  s += '#include <stdbool.h>\n'
  s += '#include <stdint.h>\n\n'

  s += 'typedef enum {\n'
  s += utils.indent('kSsMsgTypeForceSigned = -1,\n')

  for i, msg in enumerate(messages):
    s += utils.indent('kSsMsgType{} = {},\n'.format(msg.name, i))

  s += utils.indent('kSsMsgTypeUnknown = {},\n'.format(len(messages)))
  s += utils.indent('kNumSsMsgType = {},\n'.format(len(messages) + 1))
  s += '} SsMsgType;\n\n'

  s += \
'''typedef enum {
  kSsStatusForceSigned = -1,
  kSsStatusSuccess = 0,
  kSsStatusInvalidUid = 1,
  kSsStatusInvalidLen = 2,
  kNumSsStatus = 3,
} SsStatus;\n\n'''

  for t in all_types:
    d = declaration(t)
    if d:
      s += '{}\n\n'.format(d)

  for msg in messages:
    s += '#define {} {}\n'.format(packed_size_name(msg), msg.packed_size)
  s += '\n'

  s += 'SsMsgType SsInspectHeader(const uint8_t *buffer);\n'

  for msg in messages:
    s += '{};\n'.format(message_pack_prototype(msg))
    s += '{};\n'.format(message_unpack_prototype(msg))
  s += '\n'

  s += 'static const char kSsLogDelimiter[] = "SsLogFileDelimiter";\n\n'

  s += 'int SsWriteLogHeader(void *fd);\n'
  s += 'int SsFindLogDelimiter(void *fd);\n\n'

  for msg in messages:
    s += '{};\n'.format(message_log_prototype(msg))

  return s[:-1]


def c_file(spec, all_types, headers):
  messages = [x for x in all_types if isinstance(x, ss.Message)]

  s = ''

  for header in headers:
    s += '#include "{}"\n'.format(header)
  s += '\n'

  s += '#include <assert.h>\n'
  s += '#include <stdbool.h>\n'
  s += '#include <stdint.h>\n\n'

  s += '{}\n\n'.format(static_assert(all_types))

  for t in all_types:
    s += '{}\n\n'.format(pack(t))
    s += '{}\n\n'.format(unpack(t))

  s += 'static const uint32_t kMessageUids[{}] = {{\n'.format(len(messages))

  for msg in messages:
    s += utils.indent('{:#010x},\n'.format(msg.uid))

  s += '};\n\n'

  s += 'static const SsMsgType kSsMsgTypes[{}] = {{\n'.format(len(messages))

  for msg in messages:
    s += utils.indent('kSsMsgType{},\n'.format(msg.name))

  s += '};\n\n'

  s += \
'''static inline SsMsgType GetSsMsgTypeFromUid(uint32_t uid) {
  for (int32_t i = 0; i < sizeof(kMessageUids) / sizeof(kMessageUids[0]); ++i) {
    if (uid == kMessageUids[i]) {
      return kSsMsgTypes[i];
    }
  }
  return kSsMsgTypeUnknown;
}

SsMsgType SsInspectHeader(const uint8_t *buffer) {
  SsHeader header;
  SsUnpackSsHeader(buffer, &header);
  return GetSsMsgTypeFromUid(header.uid);
}\n\n'''

  s += '{}\n\n'.format(yaml_log_header(spec, messages))
  s += '{}\n\n'.format(write_log_header())
  s += '{}\n\n'.format(find_log_delimiter())

  for msg in messages:
    s += '{}\n\n'.format(message_log(msg))

  return s[:-1]


def parse_yaml(spec):
  all_types = ss.parse_yaml(spec)

  messages = [x for x in all_types if isinstance(x, ss.Message)]

  msg_type = ss.Enum('SsMsgType', 'Message types.')
  for m in messages:
    msg_type.add_value(ss.EnumValue(m.name, m.description))
  msg_type.add_value(ss.EnumValue('Unknown', 'Unknown message type.'))

  status = ss.Enum('SsStatus', 'Stuff Sack status code.')
  status.add_value(ss.EnumValue('Success', 'Success.'))
  status.add_value(ss.EnumValue('InvalidUid', 'Invalid message UID.'))
  status.add_value(ss.EnumValue('InvalidLen', 'Invalid message length.'))

  return [msg_type, status] + list(all_types)


def main():
  parser = argparse.ArgumentParser(description='Generate message pack C library.')
  parser.add_argument('--spec', required=True, help='YAML message specification.')
  parser.add_argument('--header', required=True, help='Library header file name.')
  parser.add_argument('--c_file', required=True, help='C library file name.')
  parser.add_argument('--includes', nargs='+', default=[], help='Additional includes.')
  args = parser.parse_args()

  with open(args.spec, 'r') as f:
    spec = yaml.unsafe_load(f)

  all_types = ss.parse_yaml(args.spec)

  with open(args.header, 'w') as f:
    f.write(c_header(all_types))

  with open(args.c_file, 'w') as f:
    includes = [args.header] + args.includes

    f.write(c_file(spec, all_types, includes))


if __name__ == '__main__':
  main()
