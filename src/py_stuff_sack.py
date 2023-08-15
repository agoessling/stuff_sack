import ctypes

from src import c_stuff_sack
from src import stuff_sack
from src.libc_wrapper import fopen, fclose


class UnpackError(Exception):
  pass


class UnknownMessage(UnpackError):
  pass


class IncorrectBufferSize(UnpackError):
  pass


class InvalidUid(UnpackError):
  pass


class InvalidLen(UnpackError):
  pass


class _EnumType(type(ctypes.c_int)):

  def __init__(cls, name, bases, attrs, **kwargs):
    super().__init__(name, bases, attrs, **kwargs)

    values = getattr(cls, '_values_', {})
    cls._values_ = values
    cls._rev_values_ = {v: k for k, v in values.items()}

  def __getattr__(cls, name):
    if name in cls._values_:
      return cls(cls._values_[name])

    raise AttributeError('\'{}\' class has no attribute \'{}\''.format(cls.__name__, name))

  def __len__(cls):
    return len(cls._values_)

  def __contains__(cls, item):
    if isinstance(item, str):
      return item in cls._values_

    if isinstance(item, int):
      return item in cls._rev_values_

    if hasattr(item, 'value'):
      return item.value in cls._rev_values_

    return False

  def __iter__(cls):
    return (cls(x) for x in cls._rev_values_)

  def __getitem__(cls, key):
    if key in cls._values_:
      return cls(cls._values_[key])

    raise KeyError('\'{}\' class has no key \'{}\''.format(cls.__name__, key))


class EnumMixin:

  @property
  def name(self):
    if self.value in self._rev_values_:
      return self._rev_values_[self.value]

    return 'InvalidEnumValue'

  def is_valid(self):
    return self.value in self._rev_values_

  def __str__(self):
    return '<{}.{}: {}>'.format(type(self).__name__, self.name, self.value)

  __repr__ = __str__

  def __eq__(self, other):
    return self.value == other


class _Message(ctypes.Structure):
  __slots__ = []

  @classmethod
  def get_buffer(cls):
    return (ctypes.c_uint8 * cls.packed_size)()

  @classmethod
  def unpack(cls, buf):
    if len(buf) != cls.packed_size:
      raise IncorrectBufferSize

    msg = cls()
    status = msg._unpack_func(ctypes.byref(buf), ctypes.byref(msg))

    STATUS_LIST = [
        None,
        InvalidUid,
        InvalidLen,
    ]

    if status < 0 or status >= len(STATUS_LIST):
      raise UnpackError('Unknown SsStatus enum: {}'.format(status))

    error = STATUS_LIST[status]
    if error:
      raise error

    return msg

  def pack(self):
    buf = self.get_buffer()
    self._pack_func(ctypes.byref(self), ctypes.byref(buf))
    return buf


def to_ctypes_name(name, default=None):
  ctypes_map = {
      'uint8': 'ctypes.c_uint8',
      'uint16': 'ctypes.c_uint16',
      'uint32': 'ctypes.c_uint32',
      'uint64': 'ctypes.c_uint64',
      'int8': 'ctypes.c_int8',
      'int16': 'ctypes.c_int16',
      'int32': 'ctypes.c_int32',
      'int64': 'ctypes.c_int64',
      'bool': 'ctypes.c_bool',
      'float': 'ctypes.c_float',
      'double': 'ctypes.c_double',
  }

  return ctypes_map.get(name, default)


def get_globals(library, message_spec):
  ENUM_MAP = {
      1: ctypes.c_int8,
      2: ctypes.c_int16,
      4: ctypes.c_int32,
      8: ctypes.c_int64,
  }

  BITFIELD_MAP = {
      1: ctypes.c_uint8,
      2: ctypes.c_uint16,
      4: ctypes.c_uint32,
      8: ctypes.c_uint64,
  }

  ctypes_map = {
      'uint8': ctypes.c_uint8,
      'uint16': ctypes.c_uint16,
      'uint32': ctypes.c_uint32,
      'uint64': ctypes.c_uint64,
      'int8': ctypes.c_int8,
      'int16': ctypes.c_int16,
      'int32': ctypes.c_int32,
      'int64': ctypes.c_int64,
      'bool': ctypes.c_bool,
      'float': ctypes.c_float,
      'double': ctypes.c_double,
  }

  ss_lib = ctypes.CDLL(library)

  ss_lib.SsInspectHeader.argtypes = [ctypes.c_void_p]
  ss_lib.SsInspectHeader.restype = ctypes.c_int
  ss_lib.SsWriteLogHeader.argtypes = [ctypes.c_void_p]
  ss_lib.SsWriteLogHeader.restype = ctypes.c_int
  ss_lib.SsFindLogDelimiter.argtypes = [ctypes.c_void_p]
  ss_lib.SsFindLogDelimiter.restype = ctypes.c_int

  all_types = stuff_sack.parse_yaml(message_spec)

  global_vars = {}
  msg_list = []

  for t in all_types:
    if isinstance(t, stuff_sack.Enum):
      values = {v.name: v.value for v in t.values}
      e = _EnumType(t.name, (
          EnumMixin,
          ENUM_MAP[t.bytes],
      ), {
          '_values_': values,
          '__metaclass__': _EnumType
      })
      global_vars[t.name] = e
      ctypes_map[t.name] = global_vars[t.name]

    if isinstance(t, stuff_sack.Bitfield):
      fields = [(f.name, BITFIELD_MAP[t.bytes], f.bits) for f in t.fields]
      global_vars[t.name] = type(t.name, (ctypes.Structure,), {'__slots__': [], '_fields_': fields})
      ctypes_map[t.name] = global_vars[t.name]

    if isinstance(t, stuff_sack.Struct):
      fields = []
      for field in t.fields:
        field_type = ctypes_map[field.root_type.name]

        for length in reversed(field.type.all_lengths):
          field_type *= length

        fields.append((field.name, field_type))

      attrs = {'__slots__': [], '_fields_': fields}

      base = ctypes.Structure
      if isinstance(t, stuff_sack.Message):
        base = _Message
        attrs['packed_size'] = t.packed_size
        attrs['_pack_func'] = getattr(ss_lib, c_stuff_sack.pack_function_name(t))
        attrs['_unpack_func'] = getattr(ss_lib, c_stuff_sack.unpack_function_name(t))
        attrs['_log_func'] = getattr(ss_lib, c_stuff_sack.log_function_name(t))

        attrs['_pack_func'].argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        attrs['_unpack_func'].argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        attrs['_unpack_func'].restype = ctypes.c_int
        attrs['_log_func'].argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        attrs['_log_func'].restype = ctypes.c_int

      global_vars[t.name] = type(t.name, (base,), attrs)
      ctypes_map[t.name] = global_vars[t.name]

      if isinstance(t, stuff_sack.Message):
        msg_list.append(global_vars[t.name])

  def unpack_message(buf):
    msg_type = ss_lib.SsInspectHeader(ctypes.byref(buf))
    if msg_type < 0 or msg_type >= len(msg_list):
      raise UnknownMessage("Unknown message UID.")

    return msg_list[msg_type].unpack(buf)

  global_vars['unpack_message'] = unpack_message

  class Logger:

    def __init__(self, filename):
      self.filename = filename
      self.file_p = None

    def open(self):
      self.file_p = fopen(self.filename, 'w+')
      if ss_lib.SsWriteLogHeader(self.file_p) <= 0:
        raise OSError('Could not write log header.')

    def close(self):
      if self.file_p is not None:
        fclose(self.file_p)
      self.file_p = None

    def __enter__(self):
      self.open()
      return self

    def __exit__(self, exc_type, exc_val, exc_tb):
      self.close()
      return False

    def log(self, msg):
      if self.file_p is None:
        raise RuntimeError('Log file not open.')

      if msg._log_func(self.file_p, ctypes.byref(msg)) <= 0:
        raise RuntimeError(f'Could not write {type(msg).__name__} to log.')

  global_vars['Logger'] = Logger

  # Add exceptions.
  global_vars['UnpackError'] = UnpackError
  global_vars['UnknownMessage'] = UnknownMessage
  global_vars['IncorrectBufferSize'] = IncorrectBufferSize
  global_vars['InvalidUid'] = InvalidUid
  global_vars['InvalidLen'] = InvalidLen

  return global_vars
