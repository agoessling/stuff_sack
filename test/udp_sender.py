import argparse
import math
import socket
import time

import test.test_message_def as msg_def


def MakeBitfield4BytesTest(t):
  msg = msg_def.Bitfield4BytesTest()
  msg.bitfield.field0 = int(4 * math.sin(2 * math.pi * 1 / 5 * t) + 4)
  msg.bitfield.field1 = int(4 * math.sin(2 * math.pi * 2 / 5 * t) + 4)
  msg.bitfield.field2 = int(4 * math.sin(2 * math.pi * 3 / 5 * t) + 4)
  return msg


def MakeEnum2BytesTest(t):
  msg = msg_def.Enum2BytesTest()
  msg.enumeration = int(64 * math.sin(2 * math.pi * 1 / 5 * t) + 64)
  return msg


def MakePrimitiveTest(t):
  msg = msg_def.PrimitiveTest()
  msg.uint8 = int(100 * math.sin(2 * math.pi * 1 / 5 * t) + 100)
  msg.uint16 = int(100 * math.sin(2 * math.pi * 2 / 5 * t) + 100)
  msg.uint32 = int(100 * math.sin(2 * math.pi * 3 / 5 * t) + 100)
  msg.uint64 = int(100 * math.sin(2 * math.pi * 4 / 5 * t) + 100)
  msg.int8 = int(100 * math.sin(2 * math.pi * 5 / 5 * t))
  msg.int16 = int(100 * math.sin(2 * math.pi * 6 / 5 * t))
  msg.int32 = int(100 * math.sin(2 * math.pi * 7 / 5 * t))
  msg.int64 = int(100 * math.sin(2 * math.pi * 8 / 5 * t))
  msg.boolean = math.sin(2 * math.pi * 9 / 5 * t) > 0
  msg.float_type = 100 * math.sin(2 * math.pi * 10 / 5 * t)
  msg.double_type = 100 * math.sin(2 * math.pi * 11 / 5 * t)
  return msg


def MakeArrayTest(t):
  msg = msg_def.ArrayTest()
  msg.array_1d[2].field1 = int(100 * math.sin(2 * math.pi * 1 / 5 * t) + 100)
  msg.array_2d[1][2].field1 = int(100 * math.sin(2 * math.pi * 1 / 5 * t) + 100)
  msg.array_3d[0][1][2].field1 = int(100 * math.sin(2 * math.pi * 1 / 5 * t) + 100)
  return msg


def main():
  parser = argparse.ArgumentParser(description='Send stuff_sack messages via UDP.')
  parser.add_argument('-a', '--address', default='127.0.0.1', help='Destination address')
  parser.add_argument('-p', '--port', default=9870, type=int, help='UDP port')
  parser.add_argument('-r', '--rate', default=100, type=float, help='Output rate [Hz]')
  args = parser.parse_args()

  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

  start_time = time.time()
  alarm = start_time + 1 / args.rate
  while True:
    t = time.time() - start_time

    sock.sendto(MakeBitfield4BytesTest(t).pack(), (args.address, args.port))
    sock.sendto(MakeEnum2BytesTest(t).pack(), (args.address, args.port))
    sock.sendto(MakePrimitiveTest(t).pack(), (args.address, args.port))
    sock.sendto(MakeArrayTest(t).pack(), (args.address, args.port))

    time.sleep(max(0, alarm - time.time()))
    alarm += 1 / args.rate


if __name__ == '__main__':
  main()
