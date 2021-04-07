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
  return 'Pack{}'.format(obj.name)


def unpack_function_name(obj):
  return 'Unpack{}'.format(obj.name)


def declaration(type_object):
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

  s += '}} {};\n\n'.format(bitfield.name)

  s += 'static_assert(sizeof({}) == {}, "{} size mismatch");'.format(bitfield.name, bitfield.bytes,
                                                                     bitfield.name)

  return s


def enum_declaration(enum):
  s = ''
  s += 'typedef enum {\n'
  s += _indent('k{}ForceSigned = -1,\n'.format(enum.name))

  for value in enum.values:
    s += _indent('k{}{} = {},\n'.format(enum.name, value.name, value.value))

  s += _indent('kNum{} = {},\n'.format(enum.name, len(enum.values)))
  s += '}} {};\n\n'.format(enum.name)

  s += 'static_assert(sizeof({}) == {}, "{} size mismatch");'.format(enum.name, enum.bytes,
                                                                     enum.name)

  return s


def struct_declaration(struct):
  s = ''
  s += 'typedef struct {\n'

  for field in struct.fields:
    s += _indent('{} {};\n'.format(c_type_name(field.type), c_field_name(field)))

  s += '}} {};'.format(struct.name)

  return s


def pack(type_object):
  if isinstance(type_object, mp.Primitive):
    return primitive_pack(type_object)

  if isinstance(type_object, mp.Bitfield):
    return primitive_pack(type_object)

  if isinstance(type_object, mp.Enum):
    return primitive_pack(type_object)

  if isinstance(type_object, mp.Struct):
    return struct_pack(type_object)

  raise TypeError('Unknown type: {}'.format(type(type_object)))


def unpack(type_object):
  if isinstance(type_object, mp.Primitive):
    return primitive_unpack(type_object)

  if isinstance(type_object, mp.Bitfield):
    return primitive_unpack(type_object)

  if isinstance(type_object, mp.Enum):
    return primitive_unpack(type_object)

  if isinstance(type_object, mp.Struct):
    return struct_unpack(type_object)

  raise TypeError('Unknown type: {}'.format(type(type_object)))


def primitive_pack(obj):
  bits = obj.bytes * 8
  s = ''
  s += 'static inline void {}(const {} *data, uint8_t *buffer) {{\n'.format(
      pack_function_name(obj), c_type_name(obj))
  s += _indent('uint{}_t raw_data = (uint{}_t)*data;\n'.format(bits, bits))

  for i in range(obj.bytes):
    s += _indent('buffer[{}] = (uint{}_t)(raw_data >> {});\n'.format(i, bits,
                                                                     (obj.bytes - i - 1) * 8))

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


def struct_pack(obj):
  s = ''
  s += 'static inline void {}(const {} *data, uint8_t *buffer) {{\n'.format(
      pack_function_name(obj), c_type_name(obj))

  offset = 0
  for field in obj.fields:
    if isinstance(field.type, mp.Array):
      s += _indent(array_pack(field.name, field.type, offset))
    else:
      s += _indent('{}(&data->{}, buffer + {});\n'.format(pack_function_name(field.type),
                                                          field.name, offset))

    offset += field.type.packed_size

  s += '}'

  return s


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


def struct_unpack(obj):
  return ''


def main():
  types = mp.parse_yaml('message_spec.yaml')

  for t in types:
    print(declaration(t))
    print(pack(t))
    print(unpack(t))


if __name__ == '__main__':
  main()
