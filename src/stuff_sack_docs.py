import argparse
import os.path

from src import c_stuff_sack
from src import cc_stuff_sack
from src import py_stuff_sack
from src import stuff_sack


def header(header, level=0):
  underlines = ['=', '-', '^', '"']

  if level < 0 or level >= len(underlines):
    raise ValueError('Level ({}) out of range (0, {})'.format(level, len(underlines)))

  return f'{header}\n{underlines[level] * len(header)}\n'


def c_function_doc(prototype):
  return f'.. c:function:: {prototype}\n'


def c_enum_doc(t):
  s = f'''\
{header(t.name, 3)}

.. c:enum:: {t.name}

  {t.description if t.description else ''}

  .. c:enumerator:: kForceSigned = -1
'''

  for value in t.values:
    s += f'  .. c:enumerator:: k{t.name}{value.name} = {value.value}\n'
    if value.description:
      s += f'\n    {value.description}\n'

  s += f'  .. c:enumerator:: kNum{t.name} = {len(t.values)}\n'

  return s


def c_bitfield_doc(b, domain='c'):
  s = f'''\
{header(b.name, 3)}

.. {domain}:struct:: {b.name}

  {b.description if b.description else ''}

'''

  for field in b.fields:
    s += f'  .. {domain}:var:: {c_stuff_sack.bitfield_field_declaration(b, field)}\n'
    if field.description:
      s += f'\n    {field.description}\n'

  return s


def c_alias_destination(domain, field):
  xref = ':c:type:'
  if domain == 'cpp':
    xref = ':cpp:any:'

  if field.root_type.name in [
      'uint8', 'uint16', 'uint32', 'uin64', 'int8', 'int16', 'int32', 'int64', 'bool', 'float',
      'double'
  ]:
    xref = ':samp:'

  return f'{xref}`{c_stuff_sack.c_full_type_name(field.type)}`'


def c_structure_doc(struct, domain='c'):
  s = f'''\
{header(struct.name, 3)}

.. {domain}:struct:: {struct.name}

  {struct.description if struct.description else ''}

'''
  for field in struct.fields:
    s += f'  .. {domain}:var:: {c_stuff_sack.struct_field_declaration(field, use_alias=True)}\n'
    if field.description:
      s += f'\n    {field.description}\n'
    if field.alias:
      s += f'\n    Aliased onto {c_alias_destination(domain, field)}\n'

  return s


def c_message_doc(m):
  s = f'''\
{header(m.name, 3)}

.. c:struct:: {m.name}

  {m.description if m.description else ''}

'''

  for field in m.fields:
    s += f'  .. c:var:: {c_stuff_sack.struct_field_declaration(field, use_alias=True)}\n'
    if field.description:
      s += f'\n    {field.description}\n'
    if field.alias:
      s += f'\n    Aliased onto {c_alias_destination("c", field)}\n'

  s += '\n'

  s += '.. c:macro:: {}\n\n'.format(c_stuff_sack.packed_size_name(m))

  s += '  Packed size of :c:struct:`{}` is {} bytes.\n\n'.format(m.name, m.packed_size)

  s += c_function_doc(c_stuff_sack.message_pack_prototype(m)) + '\n'
  s += '  Pack message (:c:var:`data`) into :c:var:`buffer`.\n\n'

  s += c_function_doc(c_stuff_sack.message_unpack_prototype(m)) + '\n'
  s += '  Unpack :c:var:`buffer` into message (:c:var:`data`).\n\n'

  s += c_function_doc(c_stuff_sack.message_log_prototype(m)) + '\n'
  s += '  Write message (:c:var:`data`) to log described by :c:var:`fd` (transparently passed to\n'
  s += '  :c:func:`SsWriteFile`). Returns the number of bytes written and negative values on\n'
  s += '  error.\n\n'

  return s


def cpp_enum_doc(e):
  s = f'''\
.. cpp:enum-class:: {e.name}

  {e.description if e.description else ''}'''

  for v in e.values:
    s += f'\n\n  .. cpp:enumerator:: k{v.name} = {v.value}'
    if v.description:
      s += '\n\n    ' + v.description

  return s + '\n'


def cpp_message_doc(m):
  s = f'''\
{header(m.name, 3)}

.. cpp:struct:: {m.name}

  {m.description if m.description else ''}

'''

  for f in m.fields:
    s += f'  .. cpp:member:: {c_stuff_sack.struct_field_declaration(f, use_alias=True)}\n'
    if f.description:
      s += '\n    ' + f.description + '\n'
    if f.alias:
      s += f'\n    Aliased onto {c_alias_destination("cpp", f)}\n'

  s += f'''
  .. cpp:member:: static constexpr MsgType kType = MsgType::k{m.name}

  .. cpp:member:: static constexpr uint32_t kUid = {m.uid:#010x}

  .. cpp:member:: static constexpr size_t kPackedSize = {m.packed_size}

  .. cpp:function:: static std::pair<{m.name}, Status> UnpackNew(const uint8_t *buffer)

    Unpack :c:var:`buffer` into new message and return it.

  .. cpp:function:: void Pack(uint8_t *buffer)

    Pack message into :cpp:var:`buffer`.

  .. cpp:function:: std::unique_ptr<std::array<uint8_t, kPackedSize>> Pack()

    Pack message into newly allocated buffer and return it.

  .. cpp:function:: Status Unpack(const uint8_t *buffer)

    Unpack :c:var:`buffer` into message.
'''

  return s


def py_enum_doc(e):
  s = f'''\
{header(e.name, 3)}

.. py:class:: {e.name}

  **Bases:** :py:class:`ctypes.c_int{e.bytes * 8}`

  {e.description if e.description else ''}

  **Class Attributes:**
'''

  for value in e.values:
    s += '\n'
    s += f'    .. py:attribute:: {value.name}\n'
    s += f'      :value: {value.value}\n'
    s += '\n'

    if value.description:
      s += f'\n      {value.description}\n\n'

    s += f'      Retrieves an instance of :py:class:`{e.name}`'
    s += f' initialized to value :py:attr:`~{e.name}.{value.name}`.\n'

  s += f'''
  **Instance Attributes:**

    .. py:property:: name
      :type: str

      String representation of :py:attr:`~ctypes._SimpleCData.value`.
      If the integer is not a valid value returns `\'InvalidEnumValue\'`.

  **Instance Methods:**

    .. py:method:: is_valid()

      Check if :py:attr:`~ctypes._SimpleCData.value` is a valid enum value.

      :return: a boolean representing whether integer is a valid enum value.
      :rtype: bool

'''

  return s


def py_bitfield_doc(b):
  s = f'''\
{header(b.name, 3)}

.. py:class:: {b.name}

  **Bases:** :py:class:`ctypes.Structure`

  {b.description if b.description else ''}

  **Fields:**
'''
  for field in b.fields:
    s += '\n'
    s += f'    .. py:attribute:: {field.name}\n'
    s += f'      :type: ctypes.c_uint{b.bytes * 8}\n'

    if field.description:
      s += f'\n      {field.description}\n'

    s += '\n'
    s += f'      Bits: {field.bits}\n'

  return s


def py_structure_doc(struct):
  s = f'''\
{header(struct.name, 3)}

.. py:class:: {struct.name}

  **Bases:** :py:class:`ctypes.Structure`

  {struct.description if struct.description else ''}

  **Fields:**
'''

  for field in struct.fields:
    s += '\n'
    s += f'    .. py:attribute:: {field.name}\n'

    type_name = py_stuff_sack.to_ctypes_name(field.root_type.name, field.root_type.name)

    array = ''.join(['[{}]'.format(x) for x in field.type.all_lengths])
    if array:
      array = ' ' + array

    s += f'      :type: {type_name}{array}\n'

    if field.description:
      s += f'\n      {field.description}\n'

  return s


def py_message_doc(m):
  s = f'''\
{header(m.name, 3)}

.. py:class:: {m.name}

  **Bases:** :py:class:`ctypes.Structure`

  {m.description if m.description else ''}

  **Fields:**
'''

  for field in m.fields:
    s += '\n'
    s += f'    .. py:attribute:: {field.name}\n'

    type_name = py_stuff_sack.to_ctypes_name(field.root_type.name, field.root_type.name)

    array = ''.join(['[{}]'.format(x) for x in field.type.all_lengths])
    if array:
      array = ' ' + array

    s += f'      :type: {type_name}{array}\n\n'

    if field.description:
      s += f'\n      {field.description}\n'

  s += '  **Class Attributes:**\n\n'
  s += '    .. py:attribute:: packed_size\n'
  s += f'      :value: {m.packed_size}\n\n'

  s += '  **Class Methods:**\n\n'
  s += '    .. py:method:: get_buffer()\n'
  s += '      :classmethod:\n\n'
  s += '      Get buffer of correct size for message.\n\n'
  s += f'      :return: an array of length :py:attr:`~{m.name}.packed_size`.\n'
  s += '      :rtype: ctypes.c_uint8\n\n'

  s += '    .. py:method:: unpack(buf)\n'
  s += '      :classmethod:\n\n'
  s += '      Unpack array into message.\n\n'
  s += f'      :param bytes-like buf: array of :py:attr:`~{m.name}.packed_size` '
  s += 'to be unpacked.\n'
  s += '      :return: the unpacked message.\n'
  s += f'      :rtype: {m.name}\n'
  s += '      :raises IncorrectBufferSize: unpack buffer length does not match message '
  s += f':py:attr:`~{m.name}.packed_size`.\n'
  s += '      :raises InvalidUid: UID in buffer header does not match message UID.\n'
  s += '      :raises InvalidLen: length in buffer header does not match message '
  s += f':py:attr:`~{m.name}.packed_size`.\n\n'

  s += '  **Instance Methods:**\n\n'

  s += '    .. py:method:: pack()\n\n'
  s += '      Pack message into array.\n\n'
  s += f'      :return: a packed array of length :py:attr:`~{m.name}.packed_size`.\n'
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
  *_cc
  *_py\n
''')

    f.write(header('YAML Definition', 2))
    f.write('\n.. literalinclude:: {}\n'.format(rel_spec_path))
    f.write('  :language: YAML\n\n')


def write_c_doc(directory, lib_name, yaml_file, alias_tag):
  all_types = stuff_sack.parse_yaml(yaml_file, alias_tag)

  enums = [x for x in all_types if isinstance(x, stuff_sack.Enum)]
  bitfields = [x for x in all_types if isinstance(x, stuff_sack.Bitfield)]
  structs = [
      x for x in all_types
      if isinstance(x, stuff_sack.Struct) and not isinstance(x, stuff_sack.Message)
  ]
  messages = [x for x in all_types if isinstance(x, stuff_sack.Message)]

  enums = c_stuff_sack.get_extra_enums(messages) + enums

  with open(os.path.join(directory, '{}_c.rst'.format(lib_name)), 'w') as f:
    f.write(header('{} C Documentation'.format(lib_name), 1) + '\n')

    f.write(header('Helper Functions', 2) + '\n')

    f.write('.. c:macro:: SS_HEADER_PACKED_SIZE\n\n')
    f.write('  Packed size of :c:struct:`SsHeader` is 6 bytes.\n\n')

    f.write(c_function_doc('SsMsgType SsInspectHeader(const uint8_t *buffer)') + '\n')
    f.write('  Determine message type from header in :c:var:`buffer`.\n\n')

    f.write('.. c:var:: static const char kSsLogDelimiter[]\n\n')
    f.write('  Delimiter used to separate YAML definition from binary packed data in log.\n\n')

    f.write(c_function_doc('int SsWriteFile(void *fd, const void *data, unsigned int len)') + '\n')
    f.write('  Forward declared function used by :c:func:`SsWriteLogHeader` as well as\n')
    f.write('  :c:func:`SsLogXXX` functions. This function must be defined by the user and\n')
    f.write('  linked into the library via the ``c_deps`` argument of the ``gen_message_def``\n')
    f.write('  Bazel macro.\n\n')
    f.write('  :param fd: User defined pointer referencing the file to be written. Transparently\n')
    f.write('    passed from calling functions e.g. :c:func:`SsWriteLogHeader`\n')
    f.write('  :param data: Pointer to binary data to be written to file.\n')
    f.write('  :param len: Size of data to be written in bytes.\n')
    f.write('  :returns: Number of bytes written.  Negative values indicate an error.\n\n')

    f.write(c_function_doc('int SsReadFile(void *fd, void *data, unsigned int len)') + '\n')
    f.write('  Forward declared function used by :c:func:`SsFindLogDelimiter`. This function\n')
    f.write('  must be defined by the user and linked into the library via the ``c_deps``\n')
    f.write('  argument of the ``gen_message_def`` Bazel macro.\n\n')
    f.write('  :param fd: User defined pointer referencing the file to be read. Transparently\n')
    f.write('    passed from calling functions e.g. :c:func:`SsFindLogDelimiter`\n')
    f.write('  :param data: Pointer to buffer to be written with data read from file.\n')
    f.write('  :param len: Size of data to be read in bytes.\n')
    f.write('  :returns: Number of bytes read.  Negative values indicate an error.\n\n')

    f.write(c_function_doc('int SsWriteLogHeader(void *fd)') + '\n')
    f.write('  Write YAML message definition header and delimiter to file.\n\n')
    f.write('  :param fd: User defined pointer referencing the file to be written. Transparently\n')
    f.write('    passed to :c:func:`SsWriteFile`\n')
    f.write('  :returns: Number of bytes written.  Negative values indicate an error.\n\n')

    f.write(c_function_doc('int SsFindLogDelimiter(void *fd)') + '\n')
    f.write('  Find file offset of binary data immediately following delimiter separating the\n')
    f.write('  YAML message definition from the binary log data.\n\n')
    f.write('  :param fd: User defined pointer referencing the file to be read. Transparently\n')
    f.write('    passed to :c:func:`SsReadFile`\n')
    f.write('  :returns: File offset in bytes.  Negative values indicate an error such as\n')
    f.write('    no delimiter found.\n\n')

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


def write_cc_doc(directory, lib_name, yaml_file, alias_tag):
  all_types = stuff_sack.parse_yaml(yaml_file, alias_tag)

  enums = [x for x in all_types if isinstance(x, stuff_sack.Enum)]
  bitfields = [x for x in all_types if isinstance(x, stuff_sack.Bitfield)]
  structs = [
      x for x in all_types
      if isinstance(x, stuff_sack.Struct) and not isinstance(x, stuff_sack.Message)
  ]
  messages = [x for x in all_types if isinstance(x, stuff_sack.Message)]

  enums = cc_stuff_sack.get_extra_enums(messages) + enums

  n = '\n'
  with open(os.path.join(directory, '{}_cc.rst'.format(lib_name)), 'w') as f:
    f.write(f'''\
{header(f'{lib_name} C++ Documentation', 1)}

.. cpp:namespace:: ss

{header('Helper Functions', 2)}

.. cpp:var:: static constexpr size_t kNumMsgTypes = {len(messages)}

.. cpp:var:: static constexpr size_t kHeaderPackedSize = 6

.. cpp:function:: MsgType InspectHeader(const uint8_t *buffer)

  Determine message type from header in :c:var:`buffer`.

.. cpp:type:: AnyMessage = std::variant<std::monostate, {','.join([m.name for m in messages])}>

.. cpp:function:: std::pair<AnyMessage, Status> UnpackMessage(const uint8_t *buffer, size_t len)

  Unpack :c:var:`buffer` into new message based on header and return it in the form of the
  AnyMessage variant.

.. cpp:class:: MessageDispatcher

  Dispatch class that can manage the unpacking of messages.  Users can register callbacks for
  specific messages and when found the dispatcher will call them with with a const reference to
  the newly unpacked message.

  .. cpp:function:: Status Unpack(const uint8_t *data, size_t len) const

    Attempt to unpack a message from :cpp:var:`data` and call the associated callbacks.

  .. cpp:function:: template <typename T> void AddCallback(std::function<void(const T&)> func)

    Register callback function for message type :cpp:any:`T`.

{header('Enums', 2)}

{n.join([cpp_enum_doc(e) for e in enums])}

{header('Bitfields', 2)}

{n.join([c_bitfield_doc(b, 'cpp') for b in bitfields])}

{header('Structures', 2)}

{n.join([c_structure_doc(c, 'cpp') for c in structs])}

{header('Messages', 2)}

{n.join([cpp_message_doc(m) for m in messages])}

''')


def write_py_doc(directory, lib_name, yaml_file):
  all_types = stuff_sack.parse_yaml(yaml_file, 'c')

  enums = [x for x in all_types if isinstance(x, stuff_sack.Enum)]
  bitfields = [x for x in all_types if isinstance(x, stuff_sack.Bitfield)]
  structs = [
      x for x in all_types
      if isinstance(x, stuff_sack.Struct) and not isinstance(x, stuff_sack.Message)
  ]
  messages = [x for x in all_types if isinstance(x, stuff_sack.Message)]

  enums = c_stuff_sack.get_extra_enums(messages) + enums

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
  parser.add_argument('--c_alias_tag', help='Alias tag to be used for C documentation.')
  parser.add_argument('--cc_alias_tag', help='Alias tag to be used for C++ documentation.')
  args = parser.parse_args()

  rel_output_dir = os.path.relpath(args.output_dir, args.gen_dir)
  rel_spec_path = os.path.relpath(args.spec, rel_output_dir)

  write_index_doc(args.output_dir, args.name, rel_spec_path)
  write_c_doc(args.output_dir, args.name, args.spec, args.c_alias_tag)
  write_cc_doc(args.output_dir, args.name, args.spec, args.cc_alias_tag)
  write_py_doc(args.output_dir, args.name, args.spec)


if __name__ == '__main__':
  main()
