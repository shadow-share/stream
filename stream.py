#!/usr/bin/env python
#
# Copyright (C) 2017 DouLe
#
from __future__ import print_function, with_statement
import ctypes
import socket
import struct


""" This module provides more powerful streaming data service based on socket.


"""

__all__ = ['Packet', 'Stream', 'SecureStream']


""" Exceptions Definition """


class IllegalParameter(Exception):
    pass


class StreamError(Exception):
    pass


class NotImplement(Exception):
    pass


""" Internal Methods """


def _object_to_string(class_object, **kwargs):
    name = class_object if isinstance(class_object, str) else \
        class_object.__class__.__name__

    return '<{} {}>'.format(
        name.strip('_').capitalize(),
        ' '.join([
            '{}={}'.format(k.capitalize(), v) for k, v in kwargs.items()]))


def _string_cast(_object, encoding='utf-8'):
    """ Convert Any type object to str type
    """
    if isinstance(_object, str):
        return _object
    if isinstance(_object, bytes):
        return _object.decode(encoding)
    return str(_object)


def _bytes_cast(_object, encoding='utf-8'):
    """ Convert Any type object to bytes type
    """
    if isinstance(_object, bytes):
        return _object
    if isinstance(_object, str):
        return _object.encode(encoding)
    return bytes(_object)


""" Internal Classes """


class _Bits(ctypes.Structure):
    """ C Structure

    typedef struct {
        unsigned b0 : 1;
        unsigned b1 : 1;
        unsigned b2 : 1;
        unsigned b3 : 1;
        unsigned b4 : 1;
        unsigned b5 : 1;
        unsigned b6 : 1;
        unsigned b7 : 1;
    } _Bits;
    """

    _fields_ = [
        ('b0', ctypes.c_uint8, 1), ('b1', ctypes.c_uint8, 1),
        ('b2', ctypes.c_uint8, 1), ('b3', ctypes.c_uint8, 1),
        ('b4', ctypes.c_uint8, 1), ('b5', ctypes.c_uint8, 1),
        ('b6', ctypes.c_uint8, 1), ('b7', ctypes.c_uint8, 1)
    ]

    def get_bits(self, str_type=False):
        bits = [self.b0, self.b1, self.b2, self.b3,
                self.b4, self.b5, self.b6, self.b7]
        if str_type:
            return ''.join([str(bit) for bit in bits])
        return bits

    def __str__(self):
        return _object_to_string(
            '_Bits', bits=''.join(str(b) for b in self._get_bits()))

    __repr__ = __str__


class _Byte(ctypes.Union):
    """ C union
    
    typedef union {
        _Bits bits;
        unsigned char value;
    } Byte;
    """

    _fields_ = [
        ('value', ctypes.c_char),
        ('bits', _Bits)
    ]

    _anonymous_ = ('bits',)

    def __init__(self, octet_integer):
        super(_Byte, self).__init__()

        if not isinstance(octet_integer, int):
            raise IllegalParameter('Internal error of _Byte constructor')
        self.value = octet_integer & 0xff

    def __str__(self):
        return _object_to_string(
            '_Byte', binary=self.bits.get_bits(str_type=True), value=self.value)

    __repr__ = __str__


class _ByteArray(object):

    """ Array of _Byte type
    """

    def __init__(self):
        self._byte_array = list()
        self._array_size = 0

    def append(self, byte):
        self._array_size += 1
        # TODO. using dict
        if isinstance(byte, bytes):
            return self._byte_array.append(_Byte(byte[0]))
        if isinstance(byte, str):
            return self._byte_array.append(_Byte(ord(byte[0])))
        if isinstance(byte, int):
            return self._byte_array.append(_Byte(byte))
        if isinstance(byte, _Byte):
            return self._byte_array.append(byte)
        self._array_size -= 1
        raise ValueError(
            "Byte unsupported value type '{}'".format(type(byte).__name__))

    def pop(self, index=-1):
        return self._byte_array.pop(index)

    def extend(self, data):
        if isinstance(data, (str, bytes)):
            for el in data:
                self.append(el)
        elif isinstance(data, int):
            while data:
                self.append(data)
                data >>= 8
        else:
            raise ValueError('data except str, bytes or int')

    def __getitem__(self, item):
        if isinstance(item, int):
            return self._byte_array[item]
        if isinstance(item, slice):
            return self._byte_array.__class__(self._byte_array[item])

    def __len__(self):
        return len(self._byte_array)

    def __str__(self):
        return _object_to_string(self, length=len(self))

    __repr__ = __str__


""" Publish Classes """


class Packet(_ByteArray):

    """ Binary data of packet
    """
    
    def __init__(self, fill=None):
        super(Packet, self).__init__()
        self.fill(element=fill, count=1)

    def fill(self, element=None, count=1):
        if isinstance(element, (str, bytes, int)):
            for _ in range(count):
                self.extend(element)
        else:
            raise ValueError('element except str, bytes or int type')


class Stream(object):
    """
    
    """
    def __init__(self):
        pass


class SecureStream(object):
    pass


if __name__ == '__main__':
    import unittest

    class BitsTestCase(unittest.TestCase):

        def testBitsZero(self):
            bits = _Bits(0, 0, 0, 0, 0, 0, 0, 0)
            self.assertEqual(bits.get_bits(), [0, 0, 0, 0, 0, 0, 0, 0])

        def testBitsNonZero(self):
            bits = _Bits(1, 1, 1, 1, 1, 1, 1, 1)
            self.assertEqual(bits.get_bits(), [1, 1, 1, 1, 1, 1, 1, 1])

        def testBitsStrVal(self):
            bits = _Bits(1, 0, 1, 0, 1, 0, 1, 0)
            self.assertEqual(bits.get_bits(str_type=True), '10101010')

    stream_test_suite = unittest.TestSuite()
    stream_test_suite.addTest(BitsTestCase())

    unittest.main()
