#!/usr/bin/env python
#
# Copyright (C) 2017 DouLe
#
# from __future__ import print_function, with_statement
import time
import array
import socket
import struct
import asyncio


class IllegalValue(Exception):
    pass


class SteamError(Exception):
    pass


class FamilyInvalid(Exception):
    pass


class ConnectionClosed(Exception):
    pass


RECEIVE_LENGTH = 20
BIG_ENDING = '>'
LITTLE_ENDING = '<'
NATIVE_BYTE_ORDER = LITTLE_ENDING
if struct.pack('@h', 0x0102) == b'\x01\x02':
    NATIVE_BYTE_ORDER = BIG_ENDING


class Packet(object):

    def __init__(self, encoding='utf-8'):
        self._byte_array = array.array('B')
        self._encoding = encoding

    def fill(self, contents):
        if isinstance(contents, str):
            return self._byte_array.fromstring(contents)
        if isinstance(contents, bytes):
            return self._byte_array.frombytes(contents)
        raise IllegalValue('fill method except str or bytes parameter')

    def put_uint8(self, value):
        self._byte_array.append(value & 0xff)

    def put_uint16(self, value):
        # 0x1234   --->   0x12, 0x34  [Big-Ending]
        self.put_uint8((value & 0xff00) >> 8)
        self.put_uint8(value & 0xff)

    def put_uint32(self, value):
        # 0x12345678   --->   0x12, 0x34, 0x56, 0x78  [Big-Ending]
        self.put_uint16((value & 0xffff0000) >> 16)
        self.put_uint16(value & 0xffff)

    def put_uint64(self, value):
        # 0x1122334455667788 -> 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
        self.put_uint32((value & 0xffffffff00000000) >> 32)
        self.put_uint32(value & 0xffffffff)

    def pop_uint8(self):
        return self._byte_array.pop()

    def shift_uint8(self):
        return self._byte_array.pop(0)

    def pop_uint16(self):
        # 0x1234   --->   0x12, 0x34 [Big-Ending]
        # POP: 0x34 | 0x12 << 8
        return self.pop_uint8() | self.pop_uint8() << 8

    def shift_uint16(self):
        return self.shift_uint8() << 8 | self.shift_uint8()

    def pop_uint32(self):
        # 0x11223344   --->   0x11, 0x22, 0x33, 0x44
        # POP: 0x44 | 0x33 << 8 | 0x22 << 16 | 0x11 << 24
        return self.pop_uint16() | self.pop_uint16() << 16

    def shift_uint32(self):
        return self.shift_uint16() << 16 | self.shift_uint16()

    def pop_uint64(self):
        # 0x1122334455667788 --> 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
        # POP: 0x88 | 0x77 << 8 | 0x66 << 16 | ... | 0x11 < 56
        return self.pop_uint32() | self.pop_uint32() << 32

    def shift_uint64(self):
        return self.shift_uint32() << 32 | self.shift_uint32()

    def put_string(self, string):
        # Low --------------> Hgh
        #  s   t   r   i   n   g   [Big-Ending]
        self.fill(string)

    def pop_string(self, length):
        string = self._byte_array.tobytes()[len(self) - length:]
        for _ in range(length):
            self._byte_array.pop()
        return string

    def shift_string(self, length):
        string = self._byte_array.tobytes()[:length + 1]
        for _ in range(length):
            self._byte_array.pop(0)
        return string

    def pop_all(self):
        return self.pop_string(len(self))

    def to_bytes(self):
        return self._byte_array.tobytes()

    def exists(self, subject):
        if isinstance(subject, int):
            if self._byte_array.count(subject):
                return True
        if isinstance(subject, str):
            subject = subject.encode(self._encoding)
        if isinstance(subject, bytes):
            origin = self._byte_array.tobytes()
            if origin.find(subject) != -1:
                return True
        return False

    def find(self, subject):
        if not self.exists(subject):
            return -1
        origin = self._byte_array.tobytes()
        return origin.find(subject)

    def __getitem__(self, item):
        if isinstance(item, int):
            return self._byte_array.__getitem__(item)
        if isinstance(item, slice):
            return [byte for byte in self._byte_array.__getitem__(item)]
        if isinstance(item, tuple) and len(item) == 2:
            _bytes = self.__getitem__(item[0])
            if not isinstance(_bytes, list):
                _bytes = [_bytes]

            return [
                tuple(
                    (int(b) for b in bin(byte)[2:].rjust(8, '0'))
                ).__getitem__(item[1]) for byte in _bytes]
        raise IllegalValue('Packet indices except integers or slices and tuple')

    def __len__(self):
        return len(self._byte_array)

    def __str__(self):
        return '<Packet Length={}>'.format(len(self))


class _SteamBase(object):

    def __init__(self, source=None):
        self._buffer = Packet()
        if isinstance(source, (str, bytes)):
            self._buffer.fill(source)

        self._segment_protocol = None

    def set_fragment_protocol(self, protocol, min_length=0):
        if isinstance(protocol, int):
            self._segment_protocol = \
                (min_length, lambda buf: len(buf) >= protocol)
        elif isinstance(protocol, (str, bytes)):
            if isinstance(protocol, str):
                protocol = protocol.encode('utf-8')
            self._segment_protocol = \
                (min_length, lambda buf: -1 if buf.find(protocol) == -1
                    else buf.find(protocol) + len(protocol) - 1)
        elif callable(protocol):
            self._segment_protocol = (min_length, protocol)
        else:
            raise IllegalValue(
                'fragment protocol except int, str, bytes or callable')

    def pop_fragment(self):
        min_length, protocol = self._segment_protocol
        if len(self._buffer) < min_length:
            return None
        if min_length == 0:
            min_length = len(self._buffer)
        length = protocol(self._buffer.to_bytes()[:min_length])
        string = self._buffer.shift_string(length)
        if len(string) == 0:
            return None
        return string

    def __str__(self):
        return '<StreamBase buffer_length={}>'.format(len(self._buffer))

print(time.clock())
s = Packet()
for i in range(9999999):
    s.put_uint8(0x12)
    # s.put_uint16(0x0203)
    # s.put_uint32(0x04050607)
    # s.put_uint64(0x1122334455667788)
    # s.put_string(b'abcdefg' * 16)
    # s.put_string('hijklmn' * 16)
    # s.pop_string(14)
    # s.shift_string(12)
print(time.clock())
print(s)
