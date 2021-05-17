import ctypes

from src import c_stuff_sack
from src import stuff_sack


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

    cls._values_ = getattr(cls, '_values_', {})
    for key, value in cls._values_.items():
      setattr(cls, key, value)

  def __contains__(cls, item):
    return item in cls._values_

  def __iter__(cls):
    return cls._values_.keys()

  def __len__(cls):
    return len(cls._values_)


class EnumMixin:

  def __eq__(self, other):
    return self.value == other


class _Message(ctypes.Structure):

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

  cdll = ctypes.CDLL(library)
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
      global_vars[t.name] = type(t.name, (ctypes.Structure,), {'_fields_': fields})
      ctypes_map[t.name] = global_vars[t.name]

    if isinstance(t, stuff_sack.Struct):
      fields = []
      for field in t.fields:
        field_type = ctypes_map[field.root_type.name]

        for length in reversed(field.type.all_lengths):
          field_type *= length

        fields.append((field.name, field_type))

      attrs = {'_fields_': fields}

      base = ctypes.Structure
      if isinstance(t, stuff_sack.Message):
        base = _Message
        attrs['packed_size'] = t.packed_size
        attrs['_pack_func'] = getattr(cdll, c_stuff_sack.pack_function_name(t))
        attrs['_unpack_func'] = getattr(cdll, c_stuff_sack.unpack_function_name(t))

      global_vars[t.name] = type(t.name, (base,), attrs)
      ctypes_map[t.name] = global_vars[t.name]

      if isinstance(t, stuff_sack.Message):
        msg_list.append(global_vars[t.name])

  def unpack_message(buf):
    msg_type = cdll.SsInspectHeader(ctypes.byref(buf))
    if msg_type < 0 or msg_type >= len(msg_list):
      raise UnknownMessage("Unknown message UID.")

    return msg_list[msg_type].unpack(buf)

  global_vars['unpack_message'] = unpack_message

  return global_vars
