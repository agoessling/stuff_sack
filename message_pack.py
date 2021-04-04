#!/usr/bin/env python3

import yaml


_YAML_TYPES = ['Bitfield', 'Enum', 'Struct']
_METADATA_FIELDS = ['_description']
_VALID_FIELDS = {
    'Bitfield': ['type', 'description', 'fields'],
    'Enum': ['type', 'description', 'values'],
    'Struct': ['type', 'description', 'fields'],
}
_NATIVE_TYPES = [
    'uint8',
    'uint16',
    'uint32',
    'uint64',
    'int8',
    'int16',
    'int32',
    'int64',
    'bool',
    'float',
    'double',
]


class SpecParseError(Exception):
  pass


class BitfieldField:
  def __init__(self, name, bits, description=None):
    self.name = name
    self.bits = bits
    self.description = description

  @classmethod
  def from_yaml(cls, bitfield_name, yaml):
    check_map_with_meta(bitfield_name, 'fields', yaml)
    name, bits = get_from_map_with_meta(yaml)

    if not isinstance(bits, int):
      raise SpecParseError('Value ({}) of field ({}) in {} must be a integer.'.format(
          bits, name, bitfield_name))

    if bits < 1:
      raise SpecParseError('Bit number ({}) for field ({}) in {} must be greater than zero.'.format(
          bits, name, bitfield_name))

    return cls(name, bits, yaml.get('_description'))

  def __str__(self):
    return str((self.name, self.bits, self.description))


class Bitfield:
  def __init__(self, name, description=None):
    self.name = name
    self.description = description
    self.fields = []

  def add_field(self, field):
    total_bits = sum(x.bits for x in self.fields)
    if total_bits + field.bits > 64:
      raise SpecParseError('{}:{} causes {} to exceed 64 bit limit.'.format(
          field.name, field.bits, self.name))

    self.fields.append(field)

  @classmethod
  def from_yaml(cls, name, yaml):
    check_array(name, 'fields', yaml['fields'])

    obj = cls(name, yaml.get('description'))

    for field in yaml['fields']:
      obj.add_field(BitfieldField.from_yaml(name, field))

    return obj

  def __str__(self):
    s = ''
    s += 'Name: {}\n'.format(self.name)
    s += 'Description: {}\n'.format(self.description)
    s += 'Fields:'

    for field in self.fields:
      s += '\n{}'.format(field)

    return s


def check_array(name, field, value):
  if not isinstance(value, list):
    raise SpecParseError('Value ({}) for {} in {} must be a list. (use "- entry" syntax)'.format(
        value, field, name))


def get_from_map_with_meta(m):
  for k, v in m.items():
    if not k.startswith('_'):
      return k, v

  raise KeyError('Non-metadata field not found in {}.'.format(m))


def check_map_with_meta(name, field, m):
  non_underscore_count = 0
  for k, v in m.items():
    if not isinstance(k, str):
      raise SpecParseError('Key in "{}" element ({}) in {} must be a string.'.format(
          field, k, name))

    if not k.startswith('_'):
      non_underscore_count += 1
      if non_underscore_count > 1:
        raise SpecParseError(
            'More than one non-metadata field (no leading underscore) in "{}" of {}.'.format(
                  field, name))

    elif k not in _METADATA_FIELDS:
      raise SpecParseError('Unrecognized metadata field "{}" in {}. Valid fields: {}'.format(
          k, name, _METADATA_FIELDS))

    if k == '_description' and not isinstance(v, str):
      raise SpecParseError('Value of "_description" ({}) in {} must be a string.'.format(v, name))


def check_bitfield(name, t):
  check_array(name, 'fields', t['fields'])

  bit_count = 0
  for field in t['fields']:
    check_map_with_meta(name, 'fields', field)
    key, value = get_from_map_with_meta(field)

    if not isinstance(value, int):
      raise SpecParseError('Value ({}) of field key ({}) in {} must be a integer.'.format(
          value, key, name))

    if value < 1:
      raise SpecParseError(
          'Field value ({}) for key ({}) in {} must be greater than zero.'.format(value, key, name))

    bit_count += value
    if bit_count > 64:
      raise SpecParseError('Bits in bitfield {} exceed 64 bit limit.'.format(name))


def check_enum(name, t):
  check_array(name, 'values', t['values'])

  for value in t['values']:
    check_map_with_meta(name, 'values', value)
    k, v = get_from_map_with_meta(value)

    if v is not None:
      raise SpecParseError(
          'Value for Enum ({}:{}) in {} must be null. (leave empty or use "null")'.format(
              k, v, name))


def check_struct_field_type(name, field, t, all_types):
  if not isinstance(t, (str, list)):
    raise SpecParseError((
        'Struct field value ({}:{}) in {} must be string ' +
        'or two element list (for describing arrays).').format(field, t, name))

  if isinstance(t, list):
    if len(t) != 2 or not isinstance(t[0], str) or not isinstance(t[1], int):
      raise SpecParseError(
          'Array specifier ({}) for {} in {} must be in form: [TypeName (str), size (int)].'.format(
              t, field, name))
    type_name = t[0]

    if t[1] < 1:
      raise SpecParseError(
          'Array length ({}) for {} in {} must be greater than zero.'.format(t[1], field, name))
  else:
    type_name = t

  if type_name not in _NATIVE_TYPES and type_name not in all_types:
    raise SpecParseError('Unknown type "{}" for "{}" in {}.  Valid types: {}'.format(
        type_name, field, name, _NATIVE_TYPES + list(all_types)))

  if type_name == name:
    raise SpecParseError('Recursive use of {} as type for {} in {} not allowed.'.format(
        type_name, field, name))


def check_struct(name, t, all_types):
  check_array(name, 'fields', t['fields'])

  for field in t['fields']:
    check_map_with_meta(name, 'fields', field)
    key, value = get_from_map_with_meta(field)

    check_struct_field_type(name, key, value, all_types)


def check_type(name, yaml):
  if 'type' not in yaml:
    raise SpecParseError('{} does not specify required "type" field.'.format(name))

  if yaml['type'] not in _YAML_TYPES:
    raise SpecParseError('Unrecognized type "{}" in {}. Valid types: {}'.format(
        yaml['type'], name, _YAML_TYPES))

  valid_fields = _VALID_FIELDS[yaml['type']]

  for field_name, field in yaml.items():
    if field_name not in valid_fields:
      raise SpecParseError('Unknown field "{}" in {}. Valid fields: {}'.format(
          field_name, name, valid_fields))

    if field_name == 'description' and not isinstance(field, str):
      raise SpecParseError('Description field ({}) in {} is not a string.'.format(field, name))

  if yaml['type'] == 'Bitfield':
    print(Bitfield.from_yaml(name, yaml))


#def check_type(name, t, all_types):
#  if 'type' not in t:
#    raise SpecParseError('{} does not specify required "type" field.'.format(name))
#
#  if t['type'] not in _YAML_TYPES:
#    raise SpecParseError('Unrecognized type "{}" in {}. Valid types: {}'.format(
#        t['type'], name, _YAML_TYPES))
#
#  fields = _VALID_FIELDS[t['type']]
#
#  for k, v in t.items():
#    if k not in fields:
#      raise SpecParseError('Unknown field "{}" in {}'.format(k, name))
#
#    if k == 'description' and not isinstance(v, str):
#      raise SpecParseError('Description field ({}) in {} is not a string.'.format(v, name))
#
#  if t['type'] == 'Bitfield':
#    check_bitfield(name, t)
#
#  if t['type'] == 'Enum':
#    check_enum(name, t)
#
#  if t['type'] == 'Struct':
#    check_struct(name, t, all_types)


def check_types(types):
  all_types = types.keys()
  for name, t in types.items():
    check_type(name, t)

def main():
  with open('message_spec.yaml', 'r') as f:
    types = yaml.unsafe_load(f)

  check_types(types)


if __name__ == '__main__':
  main()
