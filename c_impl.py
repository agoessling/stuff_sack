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
  if isinstance(type_object, mp.Bitfield):
    return bitfield_pack(type_object)

  if isinstance(type_object, mp.Enum):
    return enum_pack(type_object)

  if isinstance(type_object, mp.Struct):
    return struct_pack(type_object)

  raise TypeError('Unknown type: {}'.format(type(type_object)))


def main():
  types = mp.parse_yaml('message_spec.yaml')

  for t in types:
    print(declaration(t))


if __name__ == '__main__':
  main()
