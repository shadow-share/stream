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
    return '<{} {}>'.format(
        class_object.__class__.__name__.strip('_'),
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
        ('b7', ctypes.c_uint8, 1), ('b6', ctypes.c_uint8, 1),
        ('b5', ctypes.c_uint8, 1), ('b4', ctypes.c_uint8, 1),
        ('b3', ctypes.c_uint8, 1), ('b2', ctypes.c_uint8, 1),
        ('b1', ctypes.c_uint8, 1), ('b0', ctypes.c_uint8, 1)
    ]

    def get_bits(self):
        return [self.b0, self.b1, self.b2, self.b3,
                self.b4, self.b5, self.b6, self.b7]

    def __str__(self):
        return _object_to_string(
            self, bits=''.join(str(b) for b in self._get_bits()))


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
        return _object_to_string(self, binary=bin(self.value[0]))


class _ByteArray(object):

    def __init__(self):
        self._byte_array = list()
        self._array_size = 0

    def append(self, byte):
        if isinstance(byte, bytes):
            self._byte_array.append(_Byte(byte[0]))
        if isinstance(byte, str):
            self._byte_array.append(_Byte(ord(byte[0])))
        if isinstance(byte, int):
            self._byte_array.append(_Byte(byte))
        if isinstance(byte, float):
            self._byte_array.append(_Byte(int(byte)))
        print(self._byte_array[0])

    def pop(self, index=-1):
        return self._byte_array.pop(index)

    def get_byte(self, index):
        if index < 0 or index >= self._array_size:
            raise IndexError('Byte index out of range')
        return self._byte_array[index].value

    def __len__(self):
        return len(self._byte_array)

    def __str__(self):
        return _object_to_string(self, length=len(self))

    __repr__ = __str__


""" Publish Classes """


class Packet(_ByteArray):
    pass


class Stream(object):
    """
    
    """
    def __init__(self):
        pass


class SecureStream(object):
    pass


if __name__ == '__main__':

    byte_array = _ByteArray()
    byte_array.append('\xff')
    print(byte_array)
