#!/usr/bin/env python
# -*- coding: utf-8 -*-

import socket
import struct
import numpy as np
import string
import json

# --------------------------------------------
# Decorators
# --------------------------------------------

# http://stackoverflow.com/questions/5929107/python-decorators-with-parameters
# http://www.artima.com/weblogs/viewpost.jsp?thread=240845

def command(device_name, fmt=''):
    def real_command(func):
        def wrapper(self, *args, **kwargs):
            device = self.client.devices.get_device_from_name(device_name)
            device_id = int(device.id)
            cmd_id = device.get_operation_reference(func.__name__.upper())
            self.client.send_command(device_id, cmd_id, fmt, *(args + tuple(kwargs.values())))
            return func(self, *args, **kwargs)
        return wrapper
    return real_command

def write_buffer(device_name, fmt='', fmt_handshake='I', dtype=np.uint32):
    def real_command(func):
        def wrapper(self, *args, **kwargs):
            device = self.client.devices.get_device_from_name(device_name)
            device_id = int(device.id)
            cmd_id = device.get_operation_reference(func.__name__.upper())
            args_ = args[1:] + tuple(kwargs.values()) + (len(args[0]),)
            self.client.send_command(device_id, cmd_id, fmt + 'I', *args_)
            self.client.send_handshaking(args[0], fmt=fmt_handshake, dtype=dtype)
            return func(self, *args, **kwargs)
        return wrapper
    return real_command

# --------------------------------------------
# Helper functions
# --------------------------------------------

def make_command(*args):
    buff = bytearray()
    append(buff, 0, 4)        # RESERVED
    append(buff, args[0], 2)  # dev_id
    append(buff, args[1], 2)  # op_id
    # Payload
    if len(args[2:]) > 0:
        payload, payload_size = build_payload(args[2], args[3:])
        append(buff, payload_size, 4)
        buff.extend(payload)
    else:
        append(buff, 0, 4)
    return buff

def append(buff, value, size):
    if size <= 4:
        for i in reversed(range(size)):
            buff.append((value >> (8 * i)) & 0xff)
    elif size == 8:
        append(buff, value, 4)
        append(buff, value >> 32, 4)
    return size

def append_array(buff, array):
    arr_bytes = bytearray(array)
    buff += arr_bytes
    return len(arr_bytes)

# http://stackoverflow.com/questions/14431170/get-the-bits-of-a-float-in-python
def float_to_bits(f):
    return struct.unpack('>l', struct.pack('>f', f))[0]

def double_to_bits(d):
    return struct.unpack('>q', struct.pack('>d', d))[0]

def build_payload(fmt, args):
    size = 0
    payload = bytearray()
    assert len(fmt) == len(args)
    for i, type_ in enumerate(fmt):
        if type_ in ['B','b']:
            size += append(payload, args[i], 1)
        elif type_ in ['H','h']:
            size += append(payload, args[i], 2)
        elif type_ in ['I','i']:
            size += append(payload, args[i], 4)
        elif type_ in ['Q','q']:
            size += append(payload, args[i], 8)
        elif type_ is 'f':
            size += append(payload, float_to_bits(args[i]), 4)
        elif type_ is 'd':
            size += append(payload, double_to_bits(args[i]), 8)
        elif type_ is '?': # bool
            if args[i]:
                size += append(payload, 1, 1)
            else:
                size += append(payload, 0, 1)
        elif type_ is 'A':
            size += append_array(payload, args[i])
        else:
            raise ValueError('Unsupported type' + type(arg))

    return payload, size

def class_to_device_name(classname):
    """
    If the device name is in a single word DEVNAME then the associated
    class name must be Devname.

    If the device name is in a several words DEV_NAME then the associated
    class name must be DevName.
    """
    dev_name = []

    # Check whether there are capital letters within the class name
    # and insert an underscore before them
    for idx, letter in enumerate(classname):
        if idx > 0 and letter in list(string.ascii_uppercase):
            dev_name.append('_')

        dev_name.append(letter.upper())

    return ''.join(dev_name)

# --------------------------------------------
# KClient
# --------------------------------------------

class KClient:
    """ tcp-server client """

    def __init__(self, host="", port=36000, unixsock=""):
        """ Initialize connection with tcp-server

        Args:
            host: A string with the IP address
            port: Port of the TCP connection (must be an integer)
        """
        if type(host) != str:
            raise TypeError("IP address must be a string")

        if type(port) != int:
            raise TypeError("Port number must be an integer")

        self.host = host
        self.port = port
        self.unixsock = unixsock
        self.is_connected = False

        if host != "":
            try:
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

                # Prevent delayed ACK on Ubuntu
                self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 16384)
                so_rcvbuf = self.sock.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)

                #   Disable Nagle algorithm for real-time response:
                self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                tcp_nodelay = self.sock.getsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY)
                assert tcp_nodelay == 1

                # Connect to Kserver
                self.sock.connect((host, port))
                self.is_connected = True
            except socket.error as e:
                print('Failed to connect to {:s}:{:d} : {:s}'.format(host, port, e))
                self.is_connected = False
        elif unixsock != "":
            try:
                self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                self.sock.connect(unixsock)
                self.is_connected = True
            except socket.error as e:
                print('Failed to connect to unix socket address ' + unixsock)
                self.is_connected = False
        else:
            raise ValueError("Unknown socket type")

        if self.is_connected:
            self.devices = Devices(self)

    # -------------------------------------------------------
    # Send/Receive
    # -------------------------------------------------------

    def send_command(self, device_id, operation_ref, type_str='', *args):
        cmd = make_command(device_id, operation_ref, type_str, *args)
        if self.sock.send(cmd) == 0:
            raise ConnectionError("send_command(): Socket connection broken")

    def recv(self, fmt="I"):
        buff_size = struct.calcsize(fmt)
        data_recv = self.sock.recv(buff_size)
        return struct.unpack(fmt, data_recv)[0]

    def recv_uint32(self):
        return self.recv()

    def recv_uint64(self):
        return self.recv(fmt='Q')

    def recv_int32(self):
        return self.recv(fmt='i')

    def recv_float(self):
        return self.recv(fmt='f')

    def recv_double(self):
        return self.recv(fmt='d')

    def recv_bool(self):
        val = self.recv()
        return val == 1

    def recv_all(self, n_bytes):
        """ Receive exactly n_bytes bytes. """
        data = []
        n_rcv = 0
        while n_rcv < n_bytes:
            chunk = self.sock.recv(n_bytes - n_rcv)
            if chunk == '':
                break
            n_rcv += len(chunk)
            data.append(chunk)
        return b''.join(data)

    def recv_until(self, escape_seq):
        """ Receive data until an escape sequence is found. """
        total_data = []
        while 1:
            data = self.sock.recv(128).decode('utf-8')
            if data:
                total_data.append(data)
                if ''.join(total_data).find(escape_seq) > 0:
                    break
        return ''.join(total_data)

    def recv_string(self):
        return self.recv_until('\0')[:-1]

    def recv_json(self):
        return json.loads(self.recv_string())

    def recv_array(self, shape, dtype='uint32'):
        """ Receive a numpy array. """
        dtype = np.dtype(dtype)
        buff = self.recv_all(dtype.itemsize * int(np.prod(shape)))
        return np.frombuffer(buff, dtype=dtype.newbyteorder('<')).reshape(shape)

    def recv_tuple(self, fmt):
        fmt = '>' + fmt
        buff = self.recv_array(struct.calcsize(fmt), dtype='uint8')
        return tuple(struct.unpack(fmt, buff))

    def send_handshaking(self, data, fmt='I', dtype=np.uint32):
        """ Send data according to the following handshaking protocol

        1) The size of the buffer must have been sent as a
           command argument to tcp-server before
        2) tcp-server acknowledges reception readiness by sending
           the number of points to receive to the client
        3) The client sends the data buffer
        """
        data_recv = self.sock.recv(4)
        num = struct.unpack(">I", data_recv)[0]
        n_pts = len(data)

        if num == n_pts:
            fmt = ('%s'+fmt) % n_pts
            buff = struct.pack(fmt, *data.astype(dtype))
            sent = self.sock.send(buff)

            if sent == 0:
                raise ConnectionError('Failed to send buffer. Socket connection broken.')
        else:
            raise ConnectionError('Invalid handshaking')

    # -------------------------------------------------------
    # Current session information
    # -------------------------------------------------------

    def get_stats(self):
        """ Print server statistics """
        self.send_command(1, 2)
        msg = self.recv_until('EOKS')
        return msg

    def __del__(self):
        if hasattr(self, 'sock'):
            self.sock.close()

class Devices:
    def __init__(self, client):
        """ Receive and parse the commands description message sent by tcp-server """
        try:
            client.send_command(1, 1)
        except:
            raise ConnectionError('Failed to send initialization command')

        lines = client.recv_until('EOC').split('\n')
        self.devices = list(map(lambda line: Device(line), lines[1:-2]))

    def get_device_from_name(self, device_name):
        device = next((dev for dev in self.devices if dev.name == device_name), None)
        if device is None:
            raise ValueError('Device ' + device_name + ' unknown')
        return device

class Device:

    def __init__(self, line):
        self.parse_info(line)

    def parse_info(self, line):
        tokens = line.split(':')
        self.id = int(tokens[0][1:])
        self.name = tokens[1]
        self.operations = [op for op in tokens[2:] if len(op) != 0]

    def get_operation_reference(self, operation_name):
        try:
            return self.operations.index(operation_name)
        except:
            return -1
