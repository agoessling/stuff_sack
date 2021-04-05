import textwrap
import yaml


def _indent(s, level=1):
  return textwrap.indent(s, '  ' * level)


def _bytes_for_value(value, signed):
  if value < 0:
    raise ValueError('Negative values not allowed.')

  sign_bit = 1 if signed else 0
  return _bytes_for_bits(value.bit_length() + sign_bit)


def _bytes_for_bits(bits):
  if bits < 0:
    raise ValueError('Negative bits not allowed.')

  raw_bytes = (bits - 1) // 8 + 1

  for x in [1, 2, 4, 8]:
    if x >= raw_bytes:
      return x

  raise ValueError('Value greater than 8 bytes.'.format(value))


def _yaml_check_map(yaml, required_fields, optional_fields, check_valid=True):
  if not isinstance(yaml, dict):
    raise SpecParseError('{} must be a map. Use "key: value" syntax.'.format(yaml))

  for key in required_fields:
    if key not in yaml:
      raise SpecParseError('Required field "{}" not found in {}.'.format(key, yaml))

  if check_valid:
    valid_fields = required_fields + optional_fields
    for key in yaml.keys():
      if key not in valid_fields:
        raise SpecParseError('Unknown field "{}" in {}. Valid fields: {}'.format(
            key, yaml, valid_fields))


def _yaml_get_from_map_with_meta(yaml):
  for key, value in yaml.items():
    if not key.startswith('_'):
      return key, value

  raise KeyError('Non-metadata field not found in {}.'.format(yaml))


def _yaml_check_map_with_meta(yaml):
  if not isinstance(yaml, dict):
    raise SpecParseError('{} must be a map. Use "key: value" syntax.'.format(yaml))

  found_non_meta = False
  for key, value in yaml.items():
    if not isinstance(key, str):
      raise SpecParseError('Key "{}" in {} must be a string.'.format(key, yaml))

    if not key.startswith('_'):
      if found_non_meta:
        raise SpecParseError(
            'More than one non-metadata field (no leading underscore) in {}.'.format(yaml))
      found_non_meta = True
    else:
      if key == '_description':
        if not isinstance(value, str):
          raise SpecParseError('Value of "_description" ({}) in {} must be a string.'.format(
              value, yaml))
      else:
        raise SpecParseError('Unrecognized metadata field "{}" in {}.'.format(key, yaml))


def _yaml_check_array(yaml):
  if not isinstance(yaml, list):
    raise SpecParseError('Value ({}) must be a list. (use "- entry" syntax)'.format(yaml))


class SpecParseError(Exception):
  pass


class DataType:
  all_types = {}

  def __init__(self, name, description=None):
    if name in self.all_types:
      raise SpecParseError('Duplicate type name: {}'.format(name))

    self.name = name
    self.description = description

    self.all_types[name] = self

  @staticmethod
  def from_yaml(name, yaml):
    if not isinstance(name, str):
      raise SpecParseError('Type name ({}) must be a string.'.format(name))

    _yaml_check_map(yaml, ['type'], [], False)

    if 'description' in yaml and not isinstance(yaml['description'], str):
      raise SpecParseError('Description field ({}) in {} must be a string.'.format(
          yaml['description'], name))

    if yaml['type'] == 'Bitfield':
      return Bitfield.from_yaml(name, yaml)

    if yaml['type'] == 'Enum':
      return Enum.from_yaml(name, yaml)

    if yaml['type'] == 'Struct':
      return Struct.from_yaml(name, yaml)

    raise SpecParseError('Unrecognized type "{}" in {}.'.format(yaml['type'], name))

  @property
  def root_type(self):
    return self

  @classmethod
  def get_type(cls, type_name):
    if type_name not in cls.all_types:
      raise SpecParseError('Unknown type "{}".  Valid types: {}'.format(
          type_name, cls.all_types.keys()))

    return cls.all_types[type_name]


class Primitive(DataType):

  def __init__(self, name, bits):
    super().__init__(name)

    self.bits = bits


Primitive('uint8', 8),
Primitive('uint16', 16),
Primitive('uint32', 32),
Primitive('uint64', 64),
Primitive('int8', 8),
Primitive('int16', 16),
Primitive('int32', 32),
Primitive('int64', 64),
Primitive('bool', 8),
Primitive('float', 32),
Primitive('double', 64),


class Array:

  def __init__(self, elem_type, length):
    self.type = elem_type
    self.length = length

  @classmethod
  def from_yaml(cls, yaml):
    if (len(yaml) != 2 or not isinstance(yaml[0], (str, list)) or not isinstance(yaml[1], int)):
      raise SpecParseError(
          'Array specifier "{}" must be in form: [type (str, list), size (int)].'.format(yaml))

    if isinstance(yaml[0], list):
      type_object = Array.from_yaml(yaml[0])
    else:
      type_object = DataType.get_type(yaml[0])

    length = yaml[1]

    if length < 1:
      raise SpecParseError('Array length ({}) in "{}" must be greater than zero.'.format(
          length, yaml))

    return cls(type_object, length)

  @property
  def name(self):
    length_list = [self.length]
    current_type = self.type
    while isinstance(current_type, Array):
      length_list.append(current_type.length)
      current_type = current_type.type

    return current_type.name + ''.join('[{}]'.format(x) for x in length_list)

  @property
  def root_type(self):
    return self.type.root_type

  def __str__(self):
    return self.name


class BitfieldField:

  def __init__(self, name, bits, description=None):
    self.name = name
    self.bits = bits
    self.description = description

  @classmethod
  def from_yaml(cls, yaml):
    _yaml_check_map_with_meta(yaml)
    name, bits = _yaml_get_from_map_with_meta(yaml)

    if not isinstance(bits, int):
      raise SpecParseError('Value ({}) of field ({}) must be a integer.'.format(bits, name))

    if bits < 1:
      raise SpecParseError('Bit number ({}) for field ({}) must be greater than zero.'.format(
          bits, name))

    return cls(name, bits, yaml.get('_description'))

  def __str__(self):
    return str((self.name, self.bits, self.description))


class Bitfield(DataType):

  def __init__(self, name, description=None):
    super().__init__(name, description)

    self.fields = []
    self.bytes = 1

  def add_field(self, field):
    total_bits = sum(x.bits for x in self.fields) + field.bits

    if total_bits > 64:
      raise SpecParseError('{}:{} causes {} to exceed 64 bit limit.'.format(
          field.name, field.bits, self.name))

    self.bytes = _bytes_for_bits(total_bits)

    if field.name in (x.name for x in self.fields):
      raise SpecParseError('Duplicate field "{}" in {}.'.format(field.name, self.name))

    self.fields.append(field)

  @classmethod
  def from_yaml(cls, name, yaml):
    _yaml_check_map(yaml, ['type', 'fields'], ['description'])
    _yaml_check_array(yaml['fields'])

    obj = cls(name, yaml.get('description'))

    for field in yaml['fields']:
      obj.add_field(BitfieldField.from_yaml(field))

    return obj

  def __str__(self):
    s = ''
    s += 'Bitfield: {}\n'.format(self.name)
    s += 'Description: {}\n'.format(self.description)
    s += 'Fields:'

    for field in self.fields:
      s += _indent('\n{}'.format(field))

    return s


class EnumValue:

  def __init__(self, name, description=None):
    self.name = name
    self.description = description

    self.value = None

  @classmethod
  def from_yaml(cls, yaml):
    _yaml_check_map_with_meta(yaml)
    name, null = _yaml_get_from_map_with_meta(yaml)

    if null is not None:
      raise SpecParseError(
          'Value for Enum ({}:{}) must be null. (leave empty or use "null")'.format(name, null))

    return cls(name, yaml.get('_description'))

  def __str__(self):
    return str((self.name, self.description))


class Enum(DataType):

  def __init__(self, name, description=None):
    super().__init__(name, description)

    self.bytes = 1
    self.values = []

  def add_value(self, value):
    if value.name in (x.name for x in self.values):
      raise SpecParseError('Value "{}" repeated in {}.'.format(value.name, self.name))

    value.value = len(self.values)
    self.values.append(value)

    self.bytes = _bytes_for_value(len(self.values), signed=True)

  @classmethod
  def from_yaml(cls, name, yaml):
    _yaml_check_map(yaml, ['type', 'values'], ['description'])
    _yaml_check_array(yaml['values'])

    obj = cls(name, yaml.get('description'))
    for value in yaml['values']:
      obj.add_value(EnumValue.from_yaml(value))

    return obj

  def __str__(self):
    s = ''
    s += 'Enum: {}\n'.format(self.name)
    s += 'Description: {}\n'.format(self.description)
    s += 'Values:'

    for value in self.values:
      s += _indent('\n{}'.format(value))

    return s


class StructField:

  def __init__(self, name, field_type, description=None):
    self.name = name
    self.type = field_type
    self.description = description

  @classmethod
  def from_yaml(cls, yaml):
    _yaml_check_map_with_meta(yaml)
    name, field_type = _yaml_get_from_map_with_meta(yaml)

    if not isinstance(field_type, (str, list)):
      raise SpecParseError(
          ('Struct field value ({}:{}) must be string ' +
           'or two element list (for describing arrays).').format(name, field_type))

    if isinstance(field_type, list):
      type_object = Array.from_yaml(field_type)
    else:
      type_object = DataType.get_type(field_type)

    return cls(name, type_object, yaml.get('_description'))

  def __str__(self):
    return str((self.name, self.type.name, self.description))


class Struct(DataType):

  def __init__(self, name, description=None):
    super().__init__(name, description)

    self.fields = []

  def add_field(self, field):
    if field.name in (x.name for x in self.fields):
      raise SpecParseError('Duplicate field "{}" in {}.'.format(field.name, self.name))

    if field.type.root_type is self:
      raise SpecParseError('Recursive use of {} as type for {} in {} not allowed.'.format(
          self.name, field.name, self.name))

    self.fields.append(field)

  @classmethod
  def from_yaml(cls, name, yaml):
    _yaml_check_map(yaml, ['type', 'fields'], ['description'])
    _yaml_check_array(yaml['fields'])

    obj = cls(name, yaml.get('description'))
    for field in yaml['fields']:
      obj.add_field(StructField.from_yaml(field))

    return obj

  def __str__(self):
    s = ''
    s += 'Struct: {}\n'.format(self.name)
    s += 'Description: {}\n'.format(self.description)
    s += 'Fields:'

    for field in self.fields:
      s += _indent('\n{}'.format(field))

    return s


def parse_yaml(filename):
  with open(filename, 'r') as f:
    type_map = yaml.unsafe_load(f)

  _yaml_check_map(type_map, [], [], False)

  return [DataType.from_yaml(name, info) for name, info in type_map.items()]
