import ctypes

from bazel_tools.tools.python.runfiles import runfiles

_lib = ctypes.PyDLL(runfiles.Create().Rlocation('stuff_sack/src/libuid_hash-so.so'))

_lib.PrimitiveHash.argtypes = [ctypes.c_char_p, ctypes.c_int]
_lib.PrimitiveHash.restype = ctypes.c_uint32

_lib.ArrayHash.argtypes = [ctypes.c_uint32, ctypes.c_int]
_lib.ArrayHash.restype = ctypes.c_uint32

_lib.BitfieldFieldHash.argtypes = [ctypes.c_char_p, ctypes.c_int]
_lib.BitfieldFieldHash.restype = ctypes.c_uint32

_lib.BitfieldHash.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
_lib.BitfieldHash.restype = ctypes.c_uint32

_lib.EnumValueHash.argtypes = [ctypes.c_char_p, ctypes.c_int]
_lib.EnumValueHash.restype = ctypes.c_uint32

_lib.EnumHash.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
_lib.EnumHash.restype = ctypes.c_uint32

_lib.StructFieldHash.argtypes = [ctypes.c_char_p, ctypes.c_uint32]
_lib.StructFieldHash.restype = ctypes.c_uint32

_lib.StructHash.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
_lib.StructHash.restype = ctypes.c_uint32


def _wrap_name_call(hash_func):

  def func(name, *args):
    return hash_func(name.encode(), *args)

  return func


def _wrap_list_call(hash_func):

  def func(name, hash_list):
    array = (ctypes.c_uint32 * len(hash_list))(*hash_list)
    return hash_func(name.encode(), array, len(hash_list))

  return func


primitive_hash = _wrap_name_call(_lib.PrimitiveHash)
array_hash = _lib.ArrayHash
bitfield_field_hash = _wrap_name_call(_lib.BitfieldFieldHash)
bitfield_hash = _wrap_list_call(_lib.BitfieldHash)
enum_value_hash = _wrap_name_call(_lib.EnumValueHash)
enum_hash = _wrap_list_call(_lib.EnumHash)
struct_field_hash = _wrap_name_call(_lib.StructFieldHash)
struct_hash = _wrap_list_call(_lib.StructHash)
