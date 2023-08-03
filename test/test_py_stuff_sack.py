import os
import unittest

import test.test_message_def as msg_def


class TestBitfield(unittest.TestCase):

  @staticmethod
  def get_test_message():
    msg = msg_def.Bitfield4BytesTest()
    msg.bitfield.field0 = 6
    msg.bitfield.field1 = 27
    msg.bitfield.field2 = 264

    buf = msg_def.Bitfield4BytesTest.get_buffer()
    buf[6:] = [0x00, 0x01, 0x08, 0xde]

    return msg, buf

  def test_size(self):
    self.assertEqual(8, msg_def.Bitfield2BytesTest.packed_size)
    self.assertEqual(10, msg_def.Bitfield4BytesTest.packed_size)

  def test_pack_unpack(self):
    msg, buf = self.get_test_message()

    packed = msg.pack()
    self.assertEqual(buf[6:], packed[6:])

    unpacked = msg_def.Bitfield4BytesTest.unpack(packed)
    self.assertEqual(msg.bitfield.field0, unpacked.bitfield.field0)
    self.assertEqual(msg.bitfield.field1, unpacked.bitfield.field1)
    self.assertEqual(msg.bitfield.field2, unpacked.bitfield.field2)


class TestEnum(unittest.TestCase):

  @staticmethod
  def get_test_message():
    msg = msg_def.Enum2BytesTest()
    msg.enumeration = len(msg_def.Enum2Bytes)

    buf = msg_def.Enum2BytesTest.get_buffer()
    buf[6:] = [0x00, 0x80]

    return msg, buf

  def test_size(self):
    self.assertEqual(127, len(msg_def.Enum1Bytes))
    self.assertEqual(7, msg_def.Enum1BytesTest.packed_size)

    self.assertEqual(128, len(msg_def.Enum2Bytes))
    self.assertEqual(8, msg_def.Enum2BytesTest.packed_size)

  def test_pack_unpack(self):
    msg, buf = self.get_test_message()

    packed = msg.pack()
    self.assertEqual(buf[6:], packed[6:])

    unpacked = msg_def.Enum2BytesTest.unpack(packed)
    self.assertEqual(msg.enumeration, unpacked.enumeration)

  def test_enum_behavior(self):
    # Test int comparison.
    self.assertEqual(msg_def.Enum1Bytes(1), 1)

    # Test length.
    self.assertEqual(127, len(msg_def.Enum1Bytes))

    # Test __contains__.
    self.assertIn('Value0', msg_def.Enum1Bytes)
    self.assertNotIn('Value-1', msg_def.Enum1Bytes)

    self.assertIn(0, msg_def.Enum1Bytes)
    self.assertNotIn(-1, msg_def.Enum1Bytes)

    self.assertIn(msg_def.Enum1Bytes(0), msg_def.Enum1Bytes)
    self.assertNotIn(msg_def.Enum1Bytes(-1), msg_def.Enum1Bytes)

    # Test class attributes.
    self.assertIsInstance(msg_def.Enum1Bytes.Value0, msg_def.Enum1Bytes)
    self.assertEqual(msg_def.Enum1Bytes.Value1, msg_def.Enum1Bytes(1))

    with self.assertRaises(AttributeError):
      msg_def.Enum1Bytes.NoValue

    # Test string representation.
    self.assertEqual('<Enum1Bytes.Value0: 0>', str(msg_def.Enum1Bytes(0)))
    self.assertEqual('<Enum1Bytes.InvalidEnumValue: -1>', str(msg_def.Enum1Bytes(-1)))

    # Test name.
    e = msg_def.Enum1Bytes(0)
    self.assertEqual('Value0', e.name)
    e.value = 1
    self.assertEqual('Value1', e.name)
    e.value = -1
    self.assertEqual('InvalidEnumValue', e.name)

    # Test iterator.
    self.assertEqual([msg_def.Enum1Bytes(x) for x in range(len(msg_def.Enum1Bytes))],
                     list(msg_def.Enum1Bytes))

    # Test key access.
    self.assertIsInstance(msg_def.Enum1Bytes['Value0'], msg_def.Enum1Bytes)
    self.assertEqual(msg_def.Enum1Bytes.Value1, msg_def.Enum1Bytes['Value1'])

    with self.assertRaises(KeyError):
      msg_def.Enum1Bytes['NoValue']

    # Test valid.
    self.assertTrue(msg_def.Enum1Bytes(1).is_valid())
    self.assertFalse(msg_def.Enum1Bytes(-1).is_valid())

    # Test in structure.
    s = msg_def.Enum1BytesTest()
    self.assertIsInstance(s.enumeration, msg_def.Enum1Bytes)
    s.enumeration = 1
    self.assertEqual('Value1', s.enumeration.name)
    s.enumeration.value = 2
    self.assertEqual('Value2', s.enumeration.name)
    s.enumeration = msg_def.Enum1Bytes.Value10
    self.assertEqual('Value10', s.enumeration.name)


class TestPrimitive(unittest.TestCase):

  @staticmethod
  def get_test_message():
    msg = msg_def.PrimitiveTest()
    msg.uint8 = 0x01
    msg.uint16 = 0x0201
    msg.uint32 = 0x04030201
    msg.uint64 = 0x0807060504030201
    msg.int8 = 0x01
    msg.int16 = 0x0201
    msg.int32 = 0x04030201
    msg.int64 = 0x0807060504030201
    msg.boolean = True
    msg.float_type = 3.1415926
    msg.double_type = 3.1415926

    buf = msg_def.PrimitiveTest.get_buffer()
    buf[6:] = [
        0x01, # uint8, 5
        0x02, 0x01, # uint16, 7
        0x04, 0x03, 0x02, 0x01, # uint32, 9
        0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, # uint64, 13
        0x01, # int8, 21
        0x02, 0x01, # int16, 22
        0x04, 0x03, 0x02, 0x01, # int32, 24
        0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, # int64, 28
        0x01, # bool, 36
        0x40, 0x49, 0x0f, 0xda, # float, 37
        0x40, 0x09, 0x21, 0xFB, 0x4D, 0x12, 0xD8, 0x4A, # double, 41
    ]  # yapf: disable

    return msg, buf

  def test_pack_unpack(self):
    msg, buf = self.get_test_message()

    packed = msg.pack()
    self.assertEqual(buf[6:], packed[6:])

    unpacked = msg_def.PrimitiveTest.unpack(packed)
    self.assertEqual(msg.uint8, unpacked.uint8)
    self.assertEqual(msg.uint16, unpacked.uint16)
    self.assertEqual(msg.uint32, unpacked.uint32)
    self.assertEqual(msg.uint64, unpacked.uint64)
    self.assertEqual(msg.int8, unpacked.int8)
    self.assertEqual(msg.int16, unpacked.int16)
    self.assertEqual(msg.int32, unpacked.int32)
    self.assertEqual(msg.int64, unpacked.int64)
    self.assertEqual(msg.boolean, unpacked.boolean)
    self.assertEqual(msg.float_type, unpacked.float_type)
    self.assertEqual(msg.double_type, unpacked.double_type)


class TestArray(unittest.TestCase):

  @staticmethod
  def get_test_message():
    msg = msg_def.ArrayTest()

    for i, s in enumerate(msg.array_1d):
      s.field1 = i

    for i, a in enumerate(msg.array_2d):
      for j, s in enumerate(a):
        s.field1 = j + 3 * i

    for i, a in enumerate(msg.array_3d):
      for j, a in enumerate(a):
        for k, s in enumerate(a):
          s.field1 = k + 3 * j + 6 * i

    buf = msg_def.ArrayTest.get_buffer()
    buf[6:] = [
        0x00, 0x00, 0x00,
        0x00, 0x00, 0x01,
        0x00, 0x00, 0x02,
        0x00, 0x00, 0x00,
        0x00, 0x00, 0x01,
        0x00, 0x00, 0x02,
        0x00, 0x00, 0x03,
        0x00, 0x00, 0x04,
        0x00, 0x00, 0x05,
        0x00, 0x00, 0x00,
        0x00, 0x00, 0x01,
        0x00, 0x00, 0x02,
        0x00, 0x00, 0x03,
        0x00, 0x00, 0x04,
        0x00, 0x00, 0x05,
    ]  # yapf: disable

    return msg, buf

  def test_pack_unpack(self):
    msg, buf = self.get_test_message()

    packed = msg.pack()
    self.assertEqual(buf[6:], packed[6:])

    unpacked = msg_def.ArrayTest.unpack(packed)

    for i, s in enumerate(msg.array_1d):
      with self.subTest(msg='1D Array.', i=i):
        self.assertEqual(s.field0, unpacked.array_1d[i].field0)
        self.assertEqual(s.field1, unpacked.array_1d[i].field1)

    for i, a in enumerate(msg.array_2d):
      for j, s in enumerate(a):
        with self.subTest(msg='2D Array.', i=i, j=j):
          self.assertEqual(s.field0, unpacked.array_2d[i][j].field0)
          self.assertEqual(s.field1, unpacked.array_2d[i][j].field1)

    for i, a in enumerate(msg.array_3d):
      for j, a in enumerate(a):
        for k, s in enumerate(a):
          with self.subTest(msg='3D Array.', i=i, j=j, k=k):
            self.assertEqual(s.field0, unpacked.array_3d[i][j][k].field0)
            self.assertEqual(s.field1, unpacked.array_3d[i][j][k].field1)


class UnpackTest(unittest.TestCase):

  def test_unpack(self):
    msg = msg_def.PrimitiveTest()
    buf = msg.pack()

    self.assertIsInstance(msg_def.unpack_message(buf), msg_def.PrimitiveTest)

    buf[:4] = [0x00, 0x00, 0x00, 0x00]
    with self.assertRaises(msg_def.UnknownMessage):
      msg_def.unpack_message(buf)

    with self.assertRaises(msg_def.IncorrectBufferSize):
      msg_def.PrimitiveTest.unpack(buf[1:])

    with self.assertRaises(msg_def.InvalidUid):
      msg_def.PrimitiveTest.unpack(buf)

    buf = msg.pack()
    buf[5:7] = [0x00, 0x00]
    with self.assertRaises(msg_def.InvalidLen):
      msg_def.PrimitiveTest.unpack(buf)


class LoggingTest(unittest.TestCase):

  def test_logging(self):
    tmp_dir = os.environ['TEST_TMPDIR']
    temp_log = os.path.join(tmp_dir, 'test.ss')

    with msg_def.Logger(temp_log) as logger:
      logger.log(msg_def.PrimitiveTest())

    with open(temp_log, 'rb') as f:
      self.assertIn(b'SsLogFileDelimiter', f.read())


if __name__ == '__main__':
  unittest.main()
