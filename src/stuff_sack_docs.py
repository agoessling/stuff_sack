import argparse
import os.path

from src import c_stuff_sack
from src import py_stuff_sack
from src import stuff_sack


def header(header, level=0):
  underlines = ['=', '-', '^', '"']

  if level < 0 or level >= len(underlines):
    raise ValueError('Level ({}) out of range (0, {})'.format(level, len(underlines)))

  return '{}\n{}\n'.format(header, underlines[level] * len(header))


def c_function_doc(prototype):
  return '.. c:function:: {}\n'.format(prototype)


def c_enum_doc(t):
  s = ''

  s += header(t.name, 3)
  s += '\n\n'

  s += '.. c:enum:: {}\n\n'.format(t.name)

  s += '  .. c:enumerator:: kForceSigned = -1\n'

  for value in t.values:
    s += '  .. c:enumerator:: k{}{} = {}\n'.format(t.name, value.name, value.value)

  s += '  .. c:enumerator:: kNum{} = {}\n'.format(t.name, len(t.values))

  return s


def c_bitfield_doc(b):
  s = ''

  s += header(b.name, 3)
  s += '\n\n'

  s += '.. c:struct:: {}\n\n'.format(b.name)

  for field in b.fields:
    s += '  .. c:var:: {}\n'.format(c_stuff_sack.bitfield_field_declaration(b, field))

  return s


def c_structure_doc(struct):
  s = ''

  s += header(struct.name, 3)
  s += '\n\n'

  s += '.. c:struct:: {}\n\n'.format(struct.name)

  for field in struct.fields:
    s += '  .. c:var:: {}\n'.format(c_stuff_sack.struct_field_declaration(field))

  return s


def c_message_doc(m):
  s = ''

  s += header(m.name, 3)
  s += '\n\n'

  s += '.. c:struct:: {}\n\n'.format(m.name)

  for field in m.fields:
    s += '  .. c:var:: {}\n'.format(c_stuff_sack.struct_field_declaration(field))

  s += '\n'

  s += '.. c:macro:: {}\n\n'.format(c_stuff_sack.packed_size_name(m))

  s += '  Packed size of :c:struct:`{}` is {} bytes.\n\n'.format(m.name, m.packed_size)

  s += c_function_doc(c_stuff_sack.message_pack_prototype(m))
  s += '\n'
  s += '  Pack message (:c:var:`data`) into :c:var:`buffer`.\n\n'
  s += c_function_doc(c_stuff_sack.message_unpack_prototype(m))
  s += '\n'
  s += '  Unpack :c:var:`buffer` into message (:c:var:`data`).\n\n'

  return s


def py_enum_doc(e):
  s = ''
  s += header(e.name, 3)
  s += '\n\n'

  s += '.. py:class:: {}\n\n'.format(e.name)
  s += '  **Bases:** :py:class:`ctypes.c_int{}`\n\n'.format(e.bytes * 8)

  s += '  **Class Attributes:**\n'
  for value in e.values:
    s += '\n'
    s += '    .. py:attribute:: {}\n'.format(value.name)
    s += '      :value: {}\n'.format(value.value)
    s += '\n'
    s += '      Retrieves an instance of :py:class:`{}`'.format(e.name)
    s += ' initialized to value :py:attr:`~{}.{}`.\n'.format(e.name, value.name)

  s += '\n'
  s += '  **Instance Attributes:**\n\n'

  s += '    .. py:property:: name\n'
  s += '      :type: str\n\n'
  s += '      String representation of :py:attr:`~ctypes._SimpleCData.value`.\n'
  s += '      If the integer is not a valid value returns `\'InvalidEnumValue\'`.\n'

  s += '\n'
  s += '  **Instance Methods:**\n\n'

  s += '    .. py:method:: is_valid()\n\n'
  s += '      Check if :py:attr:`~ctypes._SimpleCData.value` is a valid enum value.\n\n'
  s += '      :return: a boolean representing whether integer is a valid enum value.\n'
  s += '      :rtype: bool\n\n'

  return s


def py_bitfield_doc(b):
  s = ''
  s += header(b.name, 3)
  s += '\n\n'

  s += '.. py:class:: {}\n\n'.format(b.name)
  s += '  **Bases:** :py:class:`ctypes.Structure`\n\n'

  s += '  **Fields:**\n'
  for field in b.fields:
    s += '\n'
    s += '    .. py:attribute:: {}\n'.format(field.name)
    s += '      :type: ctypes.c_uint{}\n'.format(b.bytes * 8)
    s += '\n'
    s += '      Bits: {}\n'.format(field.bits)

  return s


def py_structure_doc(struct):
  s = ''
  s += header(struct.name, 3)
  s += '\n\n'

  s += '.. py:class:: {}\n\n'.format(struct.name)
  s += '  **Bases:** :py:class:`ctypes.Structure`\n\n'

  s += '  **Fields:**\n'
  for field in struct.fields:
    s += '\n'
    s += '    .. py:attribute:: {}\n'.format(field.name)

    type_name = py_stuff_sack.to_ctypes_name(field.root_type.name, field.root_type.name)

    array = ''.join(['[{}]'.format(x) for x in field.type.all_lengths])
    if array:
      array = ' ' + array

    s += '      :type: {}{}\n'.format(type_name, array)

  return s


def py_message_doc(m):
  s = ''
  s += header(m.name, 3)
  s += '\n\n'

  s += '.. py:class:: {}\n\n'.format(m.name)
  s += '  **Bases:** :py:class:`ctypes.Structure`\n\n'

  s += '  **Fields:**\n'
  for field in m.fields:
    s += '\n'
    s += '    .. py:attribute:: {}\n'.format(field.name)

    type_name = py_stuff_sack.to_ctypes_name(field.root_type.name, field.root_type.name)

    array = ''.join(['[{}]'.format(x) for x in field.type.all_lengths])
    if array:
      array = ' ' + array

    s += '      :type: {}{}\n\n'.format(type_name, array)

  s += '  **Class Attributes:**\n\n'
  s += '    .. py:attribute:: packed_size\n'
  s += '      :value: {}\n\n'.format(m.packed_size)

  s += '  **Class Methods:**\n\n'
  s += '    .. py:method:: get_buffer()\n'
  s += '      :classmethod:\n\n'
  s += '      Get buffer of correct size for message.\n\n'
  s += '      :return: an array of length :py:attr:`~{}.packed_size`.\n'.format(m.name)
  s += '      :rtype: ctypes.c_uint8\n\n'

  s += '    .. py:method:: unpack(buf)\n'
  s += '      :classmethod:\n\n'
  s += '      Unpack array into message.\n\n'
  s += '      :param bytes-like buf: array of :py:attr:`~{}.packed_size` '.format(m.name)
  s += 'to be unpacked.\n'
  s += '      :return: the unpacked message.\n'
  s += '      :rtype: {}\n'.format(m.name)
  s += '      :raises IncorrectBufferSize: unpack buffer length does not match message '
  s += ':py:attr:`~{}.packed_size`.\n'.format(m.name)
  s += '      :raises InvalidUid: UID in buffer header does not match message UID.\n'
  s += '      :raises InvalidLen: length in buffer header does not match message'
  s += ':py:attr:`~{}.packed_size`.\n\n'.format(m.name)

  s += '  **Instance Methods:**\n\n'

  s += '    .. py:method:: pack()\n\n'
  s += '      Pack message into array.\n\n'
  s += '      :return: a packed array of length :py:attr:`~{}.packed_size`.\n'.format(m.name)
  s += '      :rtype: ctypes.c_uint8\n\n'

  return s


def write_index_doc(directory, lib_name, rel_spec_path):
  with open(os.path.join(directory, '{}_index.rst'.format(lib_name)), 'w') as f:
    f.write(header('{} Definitions'.format(lib_name), 1))

    f.write('''
.. toctree::
  :maxdepth: 2
  :caption: Contents:
  :glob:

  *_c
  *_py\n
''')

    f.write(header('YAML Definition', 2))
    f.write('\n.. literalinclude:: {}\n'.format(rel_spec_path))
    f.write('  :language: YAML\n\n')


def write_c_doc(directory, lib_name, enums, bitfields, structs, messages):
  with open(os.path.join(directory, '{}_c.rst'.format(lib_name)), 'w') as f:
    f.write(header('{} C Documentation'.format(lib_name), 1))
    f.write('\n')

    f.write(c_function_doc('SsMsgType SsInspectHeader(const uint8_t *buffer)'))
    f.write('\n')
    f.write('  Determine message type from header in :c:var:`buffer`.\n\n')

    f.write(header('Enums', 2))

    for e in enums:
      f.write('\n')
      f.write(c_enum_doc(e))

    f.write('\n')
    f.write(header('Bitfields', 2))

    for b in bitfields:
      f.write('\n')
      f.write(c_bitfield_doc(b))

    f.write('\n')
    f.write(header('Structures', 2))

    for s in structs:
      f.write('\n')
      f.write(c_structure_doc(s))

    f.write('\n')
    f.write(header('Messages', 2))

    for m in messages:
      f.write('\n')
      f.write(c_message_doc(m))


def write_py_doc(directory, lib_name, enums, bitfields, structs, messages):
  with open(os.path.join(directory, '{}_py.rst'.format(lib_name)), 'w') as f:
    f.write(header('{} Python Documentation'.format(lib_name), 1))
    f.write('\n')

    f.write('.. highlight:: python\n\n')

    f.write(header('Functions', 2))
    f.write('''\n\
.. py:function:: unpack_message(buf)\n
  Unpack buffer into message instance based on header UID.\n
  :param bytes-like buf: an array to be unpacked.
  :return: a message instance.
  :raises UnknownMessage: UID from buffer header is unknown.
  :raises Various: See individual :py:meth:`unpack` methods.\n
''')

    f.write(header('Exceptions', 2))
    f.write('''\n\
.. py:exception:: UnpackError\n
  General unpack error.\n
.. py:exception:: UnknownMessage\n
  **Bases:** :py:exc:`UnpackError`\n
  Unknown UID in buffer header.\n
.. py:exception:: IncorrectBufferSize\n
  **Bases:** :py:exc:`UnpackError`\n
  Buffer has incorrect length for message.\n
.. py:exception:: InvalidUid\n
  **Bases:** :py:exc:`UnpackError`\n
  Buffer header has incorrect UID for message.\n
.. py:exception:: InvalidLen\n
  **Bases:** :py:exc:`UnpackError`\n
  Buffer header has incorrect length for message.\n
''')

    f.write(header('Enums', 2))

    f.write('''\n\
The metaclass for enum classes includes implementations for :py:meth:`~object.__contains__`,
:py:meth:`~object.__iter__`, :py:meth:`~object.__getitem__`, and :py:meth:`~object.__len__` which
allows usage such as::

  # __contains__
  if EnumClass.Value0 in EnumClass:
    pass
  elif 'Value0' in EnumClass:
    pass
  elif 0 in EnumClass:
    pass

  # __iter__
  for value in EnumClass:
    pass

  # __getitem__
  e = EnumClass['Value0']

  # __len__
  if len(EnumClass) > 100:
    pass
''')

    for e in enums:
      f.write('\n')
      f.write(py_enum_doc(e))

    f.write('\n')
    f.write(header('Bitfields', 2))

    for b in bitfields:
      f.write('\n')
      f.write(py_bitfield_doc(b))

    f.write('\n')
    f.write(header('Structures', 2))

    for s in structs:
      f.write('\n')
      f.write(py_structure_doc(s))

    f.write('\n')
    f.write(header('Messages', 2))

    for m in messages:
      f.write('\n')
      f.write(py_message_doc(m))


def main():
  parser = argparse.ArgumentParser(description='Generate stuff_sack documentation.')
  parser.add_argument('--name', required=True, help='Library name.')
  parser.add_argument('--spec', required=True, help='YAML message specification.')
  parser.add_argument('--output_dir', required=True, help='Sphinx output directory.')
  parser.add_argument('--gen_dir', required=True, help='Sphinx root directory.')
  args = parser.parse_args()

  rel_output_dir = os.path.relpath(args.output_dir, args.gen_dir)
  rel_spec_path = os.path.relpath(args.spec, rel_output_dir)

  all_types = c_stuff_sack.parse_yaml(args.spec)
  enums = [x for x in all_types if isinstance(x, stuff_sack.Enum)]
  bitfields = [x for x in all_types if isinstance(x, stuff_sack.Bitfield)]
  structs = [
      x for x in all_types
      if isinstance(x, stuff_sack.Struct) and not isinstance(x, stuff_sack.Message)
  ]
  messages = [x for x in all_types if isinstance(x, stuff_sack.Message)]

  write_index_doc(args.output_dir, args.name, rel_spec_path)
  write_c_doc(args.output_dir, args.name, enums, bitfields, structs, messages)
  write_py_doc(args.output_dir, args.name, enums, bitfields, structs, messages)


if __name__ == '__main__':
  main()
