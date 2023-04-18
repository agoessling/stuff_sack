import ctypes
import os

libc = ctypes.CDLL('libc.so.6', use_errno=True)

libc.fopen.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
libc.fopen.restype = ctypes.c_void_p

libc.fclose.argtypes = [ctypes.c_void_p]
libc.fclose.restype = ctypes.c_int

def fopen(filename, mode):
  file_p = libc.fopen(filename.encode('utf-8'), mode.encode('utf-8'))
  if file_p is None:
    raise OSError(f'Could not open {filename}: {os.strerror(ctypes.get_errno())}')
  return file_p

def fclose(file_p):
  if libc.fclose(file_p) != 0:
    raise OSError(f'Could not close file: {os.strerror(ctypes.get_errno())}')
