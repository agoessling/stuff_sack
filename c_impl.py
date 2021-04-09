import argparse
import textwrap

import message_pack as mp


def _indent(s, level=1):
  return textwrap.indent(s, '  ' * level)


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
  while isinstance(current_type, mp.Array):
    length_list.append(current_type.length)
    current_type = current_type.type

  brackets = ''.join('[{}]'.format(x) for x in length_list)

  return '{}{}'.format(field.name, brackets)


def pack_function_name(obj):
  return 'MpPack{}'.format(obj.name)


def unpack_function_name(obj):
  return 'MpUnpack{}'.format(obj.name)


def declaration(type_object):
  if type(type_object) is mp.Primitive:
    return None

  if isinstance(type_object, mp.Bitfield):
    return bitfield_declaration(type_object)

  if isinstance(type_object, mp.Enum):
    return enum_declaration(type_object)

  if isinstance(type_object, mp.Struct):
    return struct_declaration(type_object)

  raise TypeError('Unknown type: {}'.format(type(type_object)))


def bitfield_declaration(bitfield):
  s = ''
  s += 'typedef struct {\n'

  for field in bitfield.fields:
    s += _indent('uint{}_t {} : {};\n'.format(bitfield.bytes * 8, field.name, field.bits))

  s += '}} {};'.format(bitfield.name)

  return s


def enum_declaration(enum):
  s = ''
  s += 'typedef enum {\n'
  s += _indent('k{}ForceSigned = -1,\n'.format(enum.name))

  for value in enum.values:
    s += _indent('k{}{} = {},\n'.format(enum.name, value.name, value.value))

  s += _indent('kNum{} = {},\n'.format(enum.name, len(enum.values)))
  s += '}} {};'.format(enum.name)

  return s


def struct_declaration(struct):
  s = ''
  s += 'typedef struct {\n'

  for field in struct.fields:
    s += _indent('{} {};\n'.format(c_type_name(field.type), c_field_name(field)))

  s += '}} {};'.format(struct.name)

  return s


def pack(type_object):
  if isinstance(type_object, mp.Bitfield):
    return bitfield_pack(type_object)

  if isinstance(type_object, mp.Enum):
    return primitive_pack(type_object)

  if isinstance(type_object, mp.Message):
    return message_pack(type_object)

  if isinstance(type_object, mp.Struct):
    return struct_pack(type_object)

  if isinstance(type_object, mp.Primitive):
    return primitive_pack(type_object)

  raise TypeError('Unknown type: {}'.format(type(type_object)))


def unpack(type_object):
  if isinstance(type_object, mp.Bitfield):
    return bitfield_unpack(type_object)

  if isinstance(type_object, mp.Enum):
    return primitive_unpack(type_object)

  if isinstance(type_object, mp.Message):
    return message_unpack(type_object)

  if isinstance(type_object, mp.Struct):
    return struct_unpack(type_object)

  if isinstance(type_object, mp.Primitive):
    return primitive_unpack(type_object)

  raise TypeError('Unknown type: {}'.format(type(type_object)))


def primitive_pack(obj):
  bits = obj.bytes * 8
  s = ''
  s += 'static inline void {}(const {} *data, uint8_t *buffer) {{\n'.format(
      pack_function_name(obj), c_type_name(obj))
  s += _indent('uint{}_t raw_data = (uint{}_t)*data;\n'.format(bits, bits))

  for i in range(obj.bytes):
    s += _indent('buffer[{}] = (uint8_t)(raw_data >> {});\n'.format(i, (obj.bytes - i - 1) * 8))

  s += '}'

  return s


def bitfield_pack(obj):
  bits = obj.bytes * 8
  s = ''
  s += 'static inline void {}(const {} *data, uint8_t *buffer) {{\n'.format(
      pack_function_name(obj), c_type_name(obj))
  s += _indent('union {\n')
  s += _indent('{} data;\n'.format(c_type_name(obj)), 2)
  s += _indent('uint{}_t raw_data;\n'.format(bits), 2)
  s += _indent('} data_union;\n\n')

  s += _indent('data_union.data = *data;\n')

  for i in range(obj.bytes):
    s += _indent('buffer[{}] = (uint8_t)(data_union.raw_data >> {});\n'.format(
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

  if isinstance(obj.type, mp.Array):
    s += _indent(array_pack(name, obj.type, offset_str, index_str, chr(ord(iter_var) + 1)))
  else:
    s += _indent('{}(&data->{}{}, buffer + {});\n'.format(pack_function_name(obj.root_type), name,
                                                          index_str, offset_str))

  s += '}\n'

  return s


def message_pack_prototype(obj):
  return 'void {}({} *data, uint8_t *buffer)'.format(pack_function_name(obj), c_type_name(obj))


def message_unpack_prototype(obj):
  return 'MpStatus {}(const uint8_t *buffer, {} *data)'.format(unpack_function_name(obj),
                                                               c_type_name(obj))


def message_pack(obj):
  s = ''
  s += '{} {{\n'.format(message_pack_prototype(obj))

  s += _indent('data->header.uid = {:#010x};\n'.format(obj.uid))
  s += _indent('data->header.len = {};\n\n'.format(obj.packed_size))

  s += _indent('{}\n'.format(struct_pack_body(obj)))

  s += '}'

  return s


def message_unpack(obj):
  s = ''
  s += '{} {{\n'.format(message_unpack_prototype(obj))

  s += _indent('{}(buffer + 0, &data->header);\n\n'.format(unpack_function_name(
      obj.fields[0].type)))

  s += _indent('if (data->header.uid != {:#010x}) {{\n'.format(obj.uid))
  s += _indent('return kMpStatusInvalidUid;\n', 2)
  s += _indent('}\n\n')

  s += _indent('if (data->header.len != {}) {{\n'.format(obj.packed_size))
  s += _indent('return kMpStatusInvalidLen;\n', 2)
  s += _indent('}\n\n')

  s += _indent('{}\n\n'.format(struct_unpack_body(obj, skip_header=True)))

  s += _indent('return kMpStatusSuccess;\n')
  s += '}'

  return s


def struct_pack(obj):
  s = ''
  s += 'static inline void {}(const {} *data, uint8_t *buffer) {{\n'.format(
      pack_function_name(obj), c_type_name(obj))

  s += _indent(struct_pack_body(obj))

  s += '}'

  return s


def struct_pack_body(obj):
  s = ''

  offset = 0
  for field in obj.fields:
    if isinstance(field.type, mp.Array):
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
  s += _indent('uint{}_t raw_data = 0;\n'.format(bits))

  for i in range(obj.bytes):
    s += _indent('raw_data |= (uint{}_t)buffer[{}] << {};\n'.format(bits, i,
                                                                    (obj.bytes - i - 1) * 8))

  s += _indent('*data = ({})raw_data;\n'.format(c_type_name(obj)))
  s += '}'

  return s


def bitfield_unpack(obj):
  bits = obj.bytes * 8
  s = ''
  s += 'static inline void {}(const uint8_t *buffer, {} *data) {{\n'.format(
      unpack_function_name(obj), c_type_name(obj))

  s += _indent('union {\n')
  s += _indent('{} data;\n'.format(c_type_name(obj)), 2)
  s += _indent('uint{}_t raw_data;\n'.format(bits), 2)
  s += _indent('} data_union;\n\n')

  s += _indent('data_union.raw_data = 0;\n')

  for i in range(obj.bytes):
    s += _indent('data_union.raw_data |= (uint{}_t)buffer[{}] << {};\n'.format(
        bits, i, (obj.bytes - i - 1) * 8))

  s += _indent('\n*data = data_union.data;\n')
  s += '}'

  return s


def array_unpack(name, obj, offset, index_str='', iter_var='i'):
  if ord(iter_var) > ord('z'):
    raise ValueError('Invalid iteration variable: {}'.format(iter_var))

  offset_str = '{} * {} + {}'.format(iter_var, obj.type.packed_size, offset)
  index_str += '[{}]'.format(iter_var)

  s = ''
  s += 'for(int32_t {0} = 0; {0} < {1}; ++{0}) {{\n'.format(iter_var, obj.length)

  if isinstance(obj.type, mp.Array):
    s += _indent(array_unpack(name, obj.type, offset_str, index_str, chr(ord(iter_var) + 1)))
  else:
    s += _indent('{}(buffer + {}, &data->{}{});\n'.format(unpack_function_name(obj.root_type),
                                                          offset_str, name, index_str))

  s += '}\n'

  return s


def struct_unpack(obj):
  s = ''
  s += 'static inline void {}(const uint8_t *buffer, {} *data) {{\n'.format(
      unpack_function_name(obj), c_type_name(obj))

  s += _indent('{}\n'.format(struct_unpack_body(obj)))

  s += '}'

  return s


def struct_unpack_body(obj, skip_header=False):
  s = ''

  offset = 0
  for field in obj.fields:
    if skip_header and field.type.name == 'MpHeader':
      pass
    elif isinstance(field.type, mp.Array):
      s += array_unpack(field.name, field.type, offset)
    else:
      s += '{}(buffer + {}, &data->{});\n'.format(unpack_function_name(field.type), offset,
                                                  field.name)

    offset += field.type.packed_size

  return s[:-1]


def c_header(all_types):
  messages = [x for x in all_types if isinstance(x, mp.Message)]

  s = ''
  s += '#pragma once\n\n'
  s += '#include <stdbool.h>\n'
  s += '#include <stdint.h>\n\n'

  s += 'typedef enum {\n'
  s += _indent('kMpMsgTypeForceSigned = -1,\n')

  for i, msg in enumerate(messages):
    s += _indent('kMpMsgType{} = {},\n'.format(msg.name, i))

  s += _indent('kMpMsgTypeUnknown = {},\n'.format(len(messages)))
  s += _indent('kNumMpMsgType = {},\n'.format(len(messages) + 1))
  s += '} MpMsgType;\n\n'

  s += \
'''typedef enum {
  kMpStatusForceSigned = -1,
  kMpStatusSuccess = 0,
  kMpStatusInvalidUid = 1,
  kMpStatusInvalidLen = 2,
  kNumMpStatus = 3,
} MpStatus;\n\n'''

  for t in all_types:
    d = declaration(t)
    if d:
      s += '{}\n\n'.format(d)

  s += 'MpMsgType MpInspectHeader(uint8_t *buffer);\n'

  for msg in messages:
    s += '{};\n'.format(message_pack_prototype(msg))
    s += '{};\n'.format(message_unpack_prototype(msg))

  return s[:-1]


def c_file(all_types, header):
  messages = [x for x in all_types if isinstance(x, mp.Message)]

  s = ''
  s += '#include "{}"\n\n'.format(header)

  s += '#include <stdbool.h>\n'
  s += '#include <stdint.h>\n\n'

  for t in all_types:
    s += '{}\n\n'.format(pack(t))
    s += '{}\n\n'.format(unpack(t))

  s += 'static const uint32_t kMessageUids[{}] = {{\n'.format(len(messages))

  for msg in messages:
    s += _indent('{:#010x},\n'.format(msg.uid))

  s += '};\n\n'

  s += 'static const MpMsgType kMpMsgTypes[{}] = {{\n'.format(len(messages))

  for msg in messages:
    s += _indent('kMpMsgType{},\n'.format(msg.name))

  s += '};\n\n'

  s += \
'''static inline MpMsgType GetMpMsgTypeFromUid(uint32_t uid) {
  for (int32_t i = 0; i < sizeof(kMessageUids) / sizeof(kMessageUids[0]); ++i) {
    if (uid == kMessageUids[i]) {
      return kMpMsgTypes[i];
    }
  }
  return kMpMsgTypeUnknown;
}

MpMsgType MpInspectHeader(uint8_t *buffer) {
  MpHeader header;
  MpUnpackMpHeader(buffer, &header);
  return GetMpMsgTypeFromUid(header.uid);
}'''

  return s


def main():
  parser = argparse.ArgumentParser(description='Generate message pack C library.')
  parser.add_argument('--yaml', required=True, help='YAML message specification.')
  parser.add_argument('--header', required=True, help='Library header file name.')
  parser.add_argument('--c_file', required=True, help='C library file name.')
  parser.add_argument('--header_include', help='Include string to header file.')
  args = parser.parse_args()

  all_types = mp.parse_yaml(args.yaml)

  with open(args.header, 'w') as f:
    f.write(c_header(all_types))

  with open(args.c_file, 'w') as f:
    if args.header_include:
      include = args.header_include
    else:
      include = args.header

    f.write(c_file(all_types, include))


if __name__ == '__main__':
  main()
