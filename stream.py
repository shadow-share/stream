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
        self.put_uint8((value & 0xff00) >> 8)
        self.put_uint8(value & 0xff)

    def put_uint32(self, value):
        self.put_uint16((value & 0xffff0000) >> 16)
        self.put_uint16(value & 0xffff)

    def put_uint64(self, value):
        self.put_uint32((value & 0xffffffff00000000) >> 32)
        self.put_uint32(value & 0xffffffff)

    def pop_uint8(self):
        return self._byte_array.pop()

    def pop_uint16(self):
        # 0x1234   --->   0x12, 0x34 [Big-Ending]
        # POP: 0x34 | 0x12 << 8
        return self.pop_uint8() | self.pop_uint8() << 8

    def pop_uint32(self):
        # 0x11223344   --->   0x11, 0x22, 0x33, 0x44
        # POP: 0x44 | 0x33 << 8 | 0x22 << 16 | 0x11 << 24
        return self.pop_uint16() | self.pop_uint16() << 16

    def pop_uint64(self):
        # 0x1122334455667788 --> 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
        # POP: 0x88 | 0x77 << 8 | 0x66 << 16 | ... | 0x11 < 56
        return self.pop_uint32() | self.pop_uint32() << 32

    def put_string(self, string):
        self.fill(string)

    def pop_string(self, length):
        string = self._byte_array.tobytes()[len(self) - length:]
        for _ in range(length):
            self._byte_array.pop()
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


AF_INET = socket.AF_INET
AF_INET6 = socket.AF_INET6


class Steam(object):

    def __init__(self, family=AF_INET):
        if family not in [AF_INET, AF_INET6]:
            raise FamilyInvalid('family only defined AF_INET and AF_INET6')
        self._socket_fd = socket.socket(family)
        self._buffer = Packet()

    def connect(self, host, port, block=False):
        try:
            self._socket_fd.connect((host, port))
            self._socket_fd.setblocking(block)
        except ConnectionRefusedError:
            raise SteamError('No connection could be made '
                             'because the target machine actively refused it')

    def send(self, data):
        if isinstance(data, Packet):
            data = data.to_bytes()
        if isinstance(data, str):
            data = data.encode('utf-8')
        if not isinstance(data, bytes):
            raise IllegalValue('data except str, bytes or Packet type')
        print(self._socket_fd.send(data))

    def receive(self, length=0, *, flag=None):
        self._fill_packet()
        return self._buffer.pop_all()

    def _fill_packet(self):
        buffer = self._socket_fd.recv(0)
        if buffer is None:
            print('Socket is closed')
        self._buffer.fill(buffer)


stream = Steam()
stream.connect('127.0.0.1', 8080)

get_request = Packet()
get_request.fill(b'GET / HTTP/1.1\r\n\r\n')
stream.send(get_request)
time.sleep(5)
print(stream.receive())
