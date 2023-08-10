import argparse

import test.test_message_def as msg_def


def main():
  parser = argparse.ArgumentParser(description='Generate serialized test data.')
  parser.add_argument('output', help='Output file.')
  args = parser.parse_args()

  with open(args.output, 'wb') as f:
    msg = msg_def.Bitfield4BytesTest()
    msg.bitfield.field0 = 6
    msg.bitfield.field1 = 27
    msg.bitfield.field2 = 264

    f.write(msg.pack())

    msg = msg_def.Enum2BytesTest()
    msg.enumeration = 128

    f.write(msg.pack())

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

    f.write(msg.pack())

    msg = msg_def.ArrayTest()

    msg.array_1d[0].field1 = 0
    msg.array_1d[1].field1 = 1
    msg.array_1d[2].field1 = 2

    msg.array_2d[0][0].field1 = 0
    msg.array_2d[0][1].field1 = 1
    msg.array_2d[0][2].field1 = 2
    msg.array_2d[1][0].field1 = 3
    msg.array_2d[1][1].field1 = 4
    msg.array_2d[1][2].field1 = 5

    msg.array_3d[0][0][0].field1 = 0
    msg.array_3d[0][0][1].field1 = 1
    msg.array_3d[0][0][2].field1 = 2
    msg.array_3d[0][1][0].field1 = 3
    msg.array_3d[0][1][1].field1 = 4
    msg.array_3d[0][1][2].field1 = 5

    f.write(msg.pack())


if __name__ == '__main__':
  main()
