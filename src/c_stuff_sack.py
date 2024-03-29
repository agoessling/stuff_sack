import argparse
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


def c_full_type_name(t):
  brackets = ''.join(f'[{x}]' for x in t.all_lengths)
  return c_type_name(t) + brackets


def c_field_name(field, prefix=''):
  brackets = ''.join(f'[{x}]' for x in field.type.all_lengths)

  return f'{prefix}{field.name}{brackets}'


def pack_function_name(obj):
  return f'SsPack{utils.snake_to_camel(obj.name)}'


def unpack_function_name(obj):
  return f'SsUnpack{utils.snake_to_camel(obj.name)}'


def declaration(type_object):
  if type(type_object) is ss.Primitive:
    return None

  if isinstance(type_object, ss.Bitfield):
    return bitfield_declaration(type_object)

  if isinstance(type_object, ss.Enum):
    return enum_declaration(type_object)

  if isinstance(type_object, ss.Struct):
    return struct_declaration(type_object)

  raise TypeError(f'Unknown type: {type(type_object)}')


def bitfield_c_type(bitfield):
  return f'uint{bitfield.bytes * 8}_t'


def bitfield_field_declaration(bitfield, field):
  return f'{bitfield_c_type(bitfield)} {field.name} : {field.bits}'


def bitfield_declaration(bitfield):
  s = 'typedef struct {\n'

  for field in bitfield.fields:
    s += f'  {bitfield_field_declaration(bitfield, field)};\n'

  s += f'}} {bitfield.name};'

  return s


def enum_declaration(enum):
  n = '\n'
  return f'''\
typedef enum {{
  k{enum.name}ForceSigned = -1,
{n.join([f'  k{enum.name}{v.name} = {v.value},' for v in enum.values])}
  kNum{enum.name} = {len(enum.values)},
}} {enum.name};'''


def struct_field_declaration(field, use_alias=False):
  type_name = c_type_name(field.type)
  field_name = c_field_name(field)
  if use_alias and field.alias:
    type_name = field.alias
    field_name = field.name
  return f'{type_name} {field_name}'


def struct_declaration(struct):
  n = '\n'
  return f'''\
typedef struct {{
{n.join([f'  {struct_field_declaration(f, use_alias=True)};' for f in struct.fields])}
}} {struct.name};'''


def pack(type_object):
  if isinstance(type_object, ss.Enum):
    return enum_pack(type_object)

  if isinstance(type_object, ss.Message):
    return message_pack(type_object)

  if isinstance(type_object, ss.Struct):
    return struct_pack(type_object)

  if isinstance(type_object, (ss.Primitive, ss.Bitfield)):
    return primitive_pack(type_object)

  raise TypeError('Unknown type: {}'.format(type(type_object)))


def unpack(type_object):
  if isinstance(type_object, ss.Enum):
    return enum_unpack(type_object)

  if isinstance(type_object, ss.Message):
    return message_unpack(type_object)

  if isinstance(type_object, ss.Struct):
    return struct_unpack(type_object)

  if isinstance(type_object, (ss.Primitive, ss.Bitfield)):
    return primitive_unpack(type_object)

  raise TypeError('Unknown type: {}'.format(type(type_object)))


def primitive_pack(obj):
  n = '\n'
  bits = obj.bytes * 8
  return f'''\
static inline void {pack_function_name(obj)}(const {c_type_name(obj)} *data, uint8_t *buffer) {{
  uint{bits}_t raw_data;
  memcpy(&raw_data, data, sizeof(raw_data));
{n.join([f'  buffer[{i}] = (uint8_t)(raw_data >> {(obj.bytes - i - 1) * 8});' for
    i in range(obj.bytes)])}
}}'''


def enum_pack(obj):
  bits = obj.bytes * 8
  return f'''\
static inline void {pack_function_name(obj)}(const {c_type_name(obj)} *data, uint8_t *buffer) {{
  const int{bits}_t raw_data = *data;
  SsPackInt{bits}(&raw_data, buffer);
}}'''


def array_pack(name, obj, offset, index_str='', iter_var='i'):
  if ord(iter_var) > ord('z'):
    raise ValueError('Invalid iteration variable: {}'.format(iter_var))

  offset_str = f'{iter_var} * {obj.type.packed_size} + {offset}'
  index_str += f'[{iter_var}]'

  s = f'for (int32_t {iter_var} = 0; {iter_var} < {obj.length}; ++{iter_var}) {{\n'

  if isinstance(obj.type, ss.Array):
    s += utils.indent(array_pack(name, obj.type, offset_str, index_str, chr(ord(iter_var) + 1)))
    s += '\n'
  else:
    s += f'  {pack_function_name(obj.root_type)}(&{name}{index_str}, buffer + {offset_str});\n'

  s += '}'

  return s


def message_pack_prototype(obj):
  return f'void {pack_function_name(obj)}({c_type_name(obj)} *data, uint8_t *buffer)'


def message_unpack_prototype(obj):
  return f'SsStatus {unpack_function_name(obj)}(const uint8_t *buffer, {c_type_name(obj)} *data)'


def message_pack(obj):
  return f'''\
{message_pack_prototype(obj)} {{
  data->ss_header.uid = {obj.uid:#010x};
  data->ss_header.len = {obj.packed_size};

{utils.indent(struct_pack_body(obj))}
}}'''


def message_unpack(obj):
  return f'''\
{message_unpack_prototype(obj)} {{
  {unpack_function_name(obj.fields[0].type)}(buffer + 0, &data->ss_header);

  if (data->ss_header.uid != {obj.uid:#010x}) {{
    return kSsStatusInvalidUid;
  }}

  if (data->ss_header.len != {obj.packed_size}) {{
    return kSsStatusInvalidLen;
  }}

{utils.indent(struct_unpack_body(obj))}

  return kSsStatusSuccess;
}}'''


def struct_pack_alias_body(obj):
  definitions = []
  copies = []
  for f in obj.fields:
    if not f.alias:
      continue

    addr = '&'
    if isinstance(f.type, ss.Array):
      addr = ''

    definitions.append(f'{f.type.root_type.name} {c_field_name(f, "_")};')
    copies.append(f'memcpy({addr}_{f.name}, &data->{f.name}, sizeof(_{f.name}));')

  return '\n'.join(definitions + copies)


def struct_pack(obj):
  return f'''\
static inline void {pack_function_name(obj)}(const {c_type_name(obj)} *data, uint8_t *buffer) {{
{utils.indent(struct_pack_body(obj))}
}}'''


def struct_pack_body(obj):
  s = struct_pack_alias_body(obj)
  if s:
    s += '\n\n'

  offset = 0
  for field in obj.fields:
    prefix = 'data->'
    if field.alias:
      prefix = '_'

    if isinstance(field.type, ss.Array):
      s += array_pack(prefix + field.name, field.type, offset) + '\n'
    else:
      s += f'{pack_function_name(field.type)}(&{prefix}{field.name}, buffer + {offset});\n'

    offset += field.type.packed_size

  return s[:-1]


def primitive_unpack(obj):
  n = '\n'
  bits = obj.bytes * 8
  return f'''\
static inline void {unpack_function_name(obj)}(const uint8_t *buffer, {c_type_name(obj)} *data) {{
  uint{bits}_t raw_data = 0;
{n.join([f'  raw_data |= (uint{bits}_t)buffer[{i}] << {(obj.bytes - i -1) * 8};' for
    i in range(obj.bytes)])}
  memcpy(data, &raw_data, sizeof(*data));
}}'''


def enum_unpack(obj):
  bits = obj.bytes * 8
  return f'''\
static inline void {unpack_function_name(obj)}(const uint8_t *buffer, {c_type_name(obj)} *data) {{
  int{bits}_t raw_data;
  SsUnpackInt{bits}(buffer, &raw_data);
  *data = raw_data;
}}'''


def array_unpack(name, obj, offset, index_str='', iter_var='i'):
  if ord(iter_var) > ord('z'):
    raise ValueError('Invalid iteration variable: {}'.format(iter_var))

  offset_str = f'{iter_var} * {obj.type.packed_size} + {offset}'
  index_str += f'[{iter_var}]'

  s = f'for (int32_t {iter_var} = 0; {iter_var} < {obj.length}; ++{iter_var}) {{\n'

  if isinstance(obj.type, ss.Array):
    s += utils.indent(array_unpack(name, obj.type, offset_str, index_str, chr(ord(iter_var) + 1)))
    s += '\n'
  else:
    s += f'  {unpack_function_name(obj.root_type)}(buffer + {offset_str}, &{name}{index_str});\n'

  s += '}'

  return s


def struct_unpack_alias_body(obj):
  defs = []
  copies = []
  for f in obj.fields:
    if not f.alias:
      continue

    addr = '&'
    if isinstance(f.type, ss.Array):
      addr = ''

    defs.append(f'{f.type.root_type.name} {c_field_name(f, "_")};')
    copies.append(f'memcpy((uint8_t *)&data->{f.name}, {addr}_{f.name}, sizeof(data->{f.name}));')

  return '\n'.join(defs), '\n'.join(copies)


def struct_unpack(obj):
  return f'''\
static inline void {unpack_function_name(obj)}(const uint8_t *buffer, {c_type_name(obj)} *data) {{
{utils.indent(struct_unpack_body(obj))}
}}'''


def struct_unpack_body(obj):
  s = ''

  alias_defs, alias_copies = struct_unpack_alias_body(obj)

  if alias_defs:
    s += alias_defs + '\n\n'

  offset = 0
  for field in obj.fields:
    prefix = 'data->'
    if field.alias:
      prefix = '_'

    if field.type.name == 'SsHeader':
      pass
    elif isinstance(field.type, ss.Array):
      s += array_unpack(prefix + field.name, field.type, offset) + '\n'
    else:
      s += f'{unpack_function_name(field.type)}(buffer + {offset}, &{prefix}{field.name});\n'

    offset += field.type.packed_size

  if alias_copies:
    s += '\n' + alias_copies + '\n'

  return s[:-1]


def packed_size_name(message):
  return f'SS_{utils.camel_to_snake(message.name).upper()}_PACKED_SIZE'


def static_assert(all_types, include_enums=True):
  s = ''

  for t in all_types:
    if isinstance(t, (ss.Primitive, ss.Bitfield)):
      s += f'static_assert(sizeof({c_type_name(t)}) == {t.bytes}, "{c_type_name(t)} size mismatch.");\n'
    if include_enums and isinstance(t, ss.Enum):
      s += f'static_assert(sizeof({t.name}) == sizeof(int), "Size of {t.name} not equal to int.");\n'
      s += f'static_assert(kNum{t.name} < {" * ".join(["256"] * t.bytes)} / 2, "{t.name} size mismatch.");\n'
    if isinstance(t, ss.Struct):
      for f in t.fields:
        if not f.alias:
          continue
        s += (f'static_assert(sizeof({f.alias}) == sizeof({c_full_type_name(f.type)}), ' +
              f'"{t.name}.{f.name} alias size mismatch.");\n')

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


def message_type_enum(messages):
  n = '\n'
  return f'''\
typedef enum {{
  kSsMsgTypeForceSigned = -1,
{f',{n}'.join([f'  kSsMsgType{x.name} = {i}' for i, x in enumerate(messages)])},
  kSsMsgTypeUnknown = {len(messages)},
  kNumSsMsgType = {len(messages) + 1},
}} SsMsgType;'''


def status_enum():
  return '''\
typedef enum {
  kSsStatusForceSigned = -1,
  kSsStatusSuccess = 0,
  kSsStatusInvalidUid = 1,
  kSsStatusInvalidLen = 2,
  kNumSsStatus = 3,
} SsStatus;'''


def c_header(all_types, includes):
  messages = [x for x in all_types if isinstance(x, ss.Message)]

  n = '\n'
  s = f'''\
#pragma once

#include <stdbool.h>
#include <stdint.h>

{n.join([f'#include "{inc}"' for inc in includes])}

{message_type_enum(messages)}

{status_enum()}\n\n'''

  for t in all_types:
    d = declaration(t)
    if d:
      s += f'{d}\n\n'

  for msg in messages:
    s += f'#define {packed_size_name(msg)} {msg.packed_size}\n'
  s += '#define SS_HEADER_PACKED_SIZE 6\n\n'

  s += 'SsMsgType SsInspectHeader(const uint8_t *buffer);\n'

  for msg in messages:
    s += f'{message_pack_prototype(msg)};\n'
    s += f'{message_unpack_prototype(msg)};\n'
  s += '\n'

  s += 'static const char kSsLogDelimiter[] = "SsLogFileDelimiter";\n\n'

  s += 'int SsWriteLogHeader(void *fd);\n'
  s += 'int SsFindLogDelimiter(void *fd);\n\n'

  for msg in messages:
    s += f'{message_log_prototype(msg)};\n'

  return s[:-1]


def c_file(spec, all_types, headers):
  messages = [x for x in all_types if isinstance(x, ss.Message)]

  s = ''

  for header in headers:
    s += '#include "{}"\n'.format(header)
  s += '\n'

  s += f'''\
#include <assert.h>
#include <stdbool.h>
#include <stdalign.h>
#include <stdint.h>
#include <string.h>

{static_assert(all_types)}\n\n'''

  for t in all_types:
    s += '{}\n\n'.format(pack(t))
    s += '{}\n\n'.format(unpack(t))

  s += 'static const uint32_t kMessageUids[{}] = {{\n'.format(len(messages))

  for msg in messages:
    s += f'  {msg.uid:#010x},\n'

  s += '};\n\n'

  s += f'static const SsMsgType kSsMsgTypes[{len(messages)}] = {{\n'

  for msg in messages:
    s += f'  kSsMsgType{msg.name},\n'

  s += '};\n\n'

  s += '''\
static inline SsMsgType GetSsMsgTypeFromUid(uint32_t uid) {
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

  s += yaml_log_header(spec, messages) + '\n\n'
  s += write_log_header() + '\n\n'
  s += find_log_delimiter() + '\n\n'

  for msg in messages:
    s += message_log(msg) + '\n\n'

  return s[:-1]


def get_extra_enums(messages):
  msg_type = ss.Enum('SsMsgType', 'Message types.')
  for m in messages:
    msg_type.add_value(ss.EnumValue(m.name, m.description))
  msg_type.add_value(ss.EnumValue('Unknown', 'Unknown message type.'))

  status = ss.Enum('SsStatus', 'Stuff Sack status code.')
  status.add_value(ss.EnumValue('Success', 'Success.'))
  status.add_value(ss.EnumValue('InvalidUid', 'Invalid message UID.'))
  status.add_value(ss.EnumValue('InvalidLen', 'Invalid message length.'))

  return [msg_type, status]


def main():
  parser = argparse.ArgumentParser(description='Generate message pack C library.')
  parser.add_argument('--spec', required=True, help='YAML message specification.')
  parser.add_argument('--source', required=True, help='Library source file name.')
  parser.add_argument('--header', required=True, help='Library header file name.')
  parser.add_argument('--includes', nargs='+', default=[], help='Additional includes.')
  parser.add_argument('--alias_tag', help='Alias tag to be used for generation.')
  args = parser.parse_args()

  with open(args.spec, 'r') as f:
    spec = yaml.unsafe_load(f)

  all_types = ss.parse_yaml(args.spec, args.alias_tag)

  with open(args.header, 'w') as f:
    f.write(c_header(all_types, args.includes))

  with open(args.source, 'w') as f:
    includes = [args.header] + args.includes

    f.write(c_file(spec, all_types, includes))


if __name__ == '__main__':
  main()
