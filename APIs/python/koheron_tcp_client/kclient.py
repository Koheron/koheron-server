import socket
import select
import struct
import time
import math
import numpy as np
import functools
import string
import sys
import traceback

# --------------------------------------------
# Decorators
# --------------------------------------------

# http://stackoverflow.com/questions/5929107/python-decorators-with-parameters
# http://www.artima.com/weblogs/viewpost.jsp?thread=240845

def command(device_name):
    def real_command(func):
        """ Decorate commands

        If the name of the command is CMD_NAME,
        then the name of the function must be cmd_name.

        /!\ The order of kwargs matters.
        """
        @functools.wraps(func)
        def wrapper(self, *args, **kwargs):
            params = self.client.cmds.get_device(device_name)
            device_id = str(params.id)
            cmd_id = params.get_op_ref(func.__name__.upper())
            self.client.send_command(device_id, cmd_id, *(args + tuple(kwargs.values())))
            return func(self, *args, **kwargs)
        return wrapper
    return real_command

def write_buffer(device_name, format_char='I', dtype=np.uint32):
    def command_wrap(func):
        def wrapper(self, *args, **kwargs):
            params = self.client.cmds.get_device(device_name)
            device_id = str(params.id)
            cmd_id = params.get_op_ref(func.__name__.upper())
            args_ = args[1:] + tuple(kwargs.values()) + (len(args[0]),)
            self.client.send_command(device_id, cmd_id, *args_)
            self.client.send_handshaking(args[0], format_char=format_char, dtype=dtype)
            func(self, *args, **kwargs)
        return wrapper
    return command_wrap

# --------------------------------------------
# Helper functions
# --------------------------------------------


def make_command(*args):
    # http://stackoverflow.com/questions/18310152/sending-binary-data-over-sockets-with-python
    args_str = "|".join([str(arg) for arg in args[2:-1]])+'|\n'
    print args[0]
    print args[1]
    print args_str

    buff = bytearray()
    _append_u32(buff, 0)                        # RESERVED
    _append_u16(buff, args[0])                  # dev_id
    _append_u16(buff, args[1])                  # op_id
    _append_u32(buff, sys.getsizeof(args_str))  # payload_size

    buff.extend(bytes(args_str))
    return buff

def _append_u16(buff, value):
    buff.append((value >> 8) & 0xff)
    buff.append(value & 0xff)

def _append_u32(buff, value):
    buff.append((value >> 24) & 0xff)
    buff.append((value >> 16) & 0xff)
    buff.append((value >> 8) & 0xff)
    buff.append(value & 0xff)

def reference_dict(self):
    params = self.client.cmds.get_device(_class_to_device_name
                                         (self.__class__.__name__))
    ref_dict = {'id': str(params.id)}
    for method in dir(self):
        if callable(getattr(self, method)):
            op_ref = params.get_op_ref(method.upper())
            if op_ref >= 0:
                ref_dict[method] = str(params.get_op_ref(method.upper()))
    return ref_dict


def _class_to_device_name(classname):
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
    """ KServer client

    Initializes the connection with KServer, then retrieves
    the current configuration: that is the available devices and
    the commands associated.

    It is also in charge of reception/emission of data with KServer
    """

    def __init__(self, host, port=36000, verbose=False, timeout=2.0):
        """ Initialize connection with KServer

        Args:
            host: A string with the IP address
            port: Port of the TCP connection (must be an integer)
            verbose: To display the retrieved KServer configuration
        """
        if type(host) != str:
            raise TypeError("IP address must be a string")

        if type(port) != int:
            raise TypeError("Port number must be an integer")

        self.host = host
        self.port = port
        self.verbose = verbose
        self.is_connected = False
        self.timeout = timeout

        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # self.sock.settimeout(timeout)

            #   Disable Nagle algorithm for real-time response:
            self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            # Connect to Kserver
            self.sock.connect((host, port))
            self.is_connected = True
        except socket.error as e:
            print('Failed to connect to {:s}:{:d} : {:s}'
                  .format(host, port, e))

        if self.is_connected:
            self._get_commands()

    def _get_commands(self):
        self.cmds = Commands(self)

        if not self.cmds.success:
            # Wait a bit and retry
            time.sleep(0.1)
            self.cmds = Commands(self)

            if not self.cmds.success:
                self.is_connected = False
                self.sock.close()

    # -------------------------------------------------------
    # Send/Receive
    # -------------------------------------------------------

    def send(self, cmd):
        """ Send a command

        Args:
            cmd: The command to be send

        Raise RuntimeError if broken connection.
        """
        # sent = self.sock.send(cmd.encode('utf-8'))
        for char in cmd:
            print char
        sent = self.sock.send(cmd)

        if sent == 0:
            raise RuntimeError("kclient-send: Socket connection broken")

    def send_command(self, device_id, operation_ref, *args):
        self.send(make_command(device_id, operation_ref, *args))

    def recv_int(self, buff_size, err_msg=None, fmt="I"):
        """ Receive an integer

        Args:
            buff_size: Maximum amount of data to be received at once
            err_msg: Error message. If you require the server to
                     send an message signaling an error occured and
                     that no data can be retrieve.

        Raise RuntimeError on error.
        """
        try:
            ready = select.select([self.sock], [], [], self.timeout)

            if ready[0]:
                data_recv = self.sock.recv(buff_size)
                if data_recv == '':
                    raise RuntimeError("kclient-recv_int: Socket connection broken")

                if len(data_recv) != buff_size:
                    raise RuntimeError("kclient-recv_int: Invalid size received")

                if err_msg is not None:
                    if data_recv[:len(err_msg)] == err_msg:
                        raise RuntimeError("kclient-recv_int: No data available")
						
                return struct.unpack(fmt, data_recv)[0]
        except:
            raise RuntimeError("kclient-recv_int: Reception error")

    def recv_n_bytes(self, n_bytes):
        """ Receive exactly n bytes

        Args:
            n_bytes: Number of bytes to receive
        """
        data = []
        n_rcv = 0

        while n_rcv < n_bytes:
            try:
                chunk = self.sock.recv(n_bytes - n_rcv)

                if chunk == '':
                    break

                n_rcv = n_rcv + len(chunk)
                data.append(chunk)
            except:
                print("recv_n_bytes: sock.recv failed")
                return ''

        return b''.join(data)

    def recv_timeout(self, escape_seq, timeout=5):
        """
        Receive data until either an escape sequence
        is found or a timeout is exceeded

        Args:
            socket: The socket used for communication
            escape_seq: String containing the escape sequence. Ex. 'EOF'.
            timeout: Reception timeout in seconds
        """

        # total data partwise in an array
        total_data = [];
        data = '';

        begin = time.time()

        while 1:
            if time.time()-begin > timeout:
                print("Error in recv_timeout Timeout exceeded")
                return "RECV_ERR_TIMEOUT"

            try:
                data = self.sock.recv(2048).decode('utf-8')

                if data:
                    total_data.append(data)
                    begin = time.time()

                    if data.find(escape_seq) > 0:
                        break
            except:
                pass

            # To avoid the program to freeze at connection sometimes
            time.sleep(0.005)

        return ''.join(total_data)

    def recv_buffer(self, buff_size, data_type='uint32'):
        """ Receive a buffer of uint32

        Args:
            buff_size Number of samples to receive

        Return: A Numpy array with the data on success.

        Raise a RuntimeError on reception failure
        """
        np_dtype = np.dtype(data_type)
        buff = self.recv_n_bytes(np_dtype.itemsize * buff_size)

        if buff == '':
            raise RuntimeError("recv_buffer: reception failed")

        np_dtype = np_dtype.newbyteorder('<')
        data = np.frombuffer(buff, dtype=np_dtype)
        return data

    def recv_tuple(self):
        tmp_buffer = [' ']
        res_tuple = []

        while tmp_buffer[-1] != '\n':
            char = self.recv_n_bytes(1)

            if char == ':':
                toks = ''.join(tmp_buffer[1:]).split('@')

                # Receive an unwanted unicode character on the first char
                # We thus clean all unicode characters
                elmt_type = toks[0].decode('unicode_escape')\
                .encode('ascii', 'ignore').strip()

                if elmt_type == 'i' or elmt_type == 'j':
                    res_tuple.append(int(toks[1]))
                elif elmt_type == 'f':
                    res_tuple.append(float(toks[1]))
                else:  # String
                    res_tuple.append(toks[1])

                tmp_buffer = [' ']
            else:
                tmp_buffer.append(char)

        return tuple(res_tuple)

    def send_handshaking(self, data, format_char='I', dtype=np.uint32):
        """ Send data with handshaking protocol

        1) The size of the buffer must have been send as a
           command argument to KServer before
        2) KServer acknowledges reception readiness by sending
           the number of points to receive to the client
        3) The client send the data buffer

        Args:
            data: The data buffer to be sent
            format_char: format character, unsigned int by default
            (https://docs.python.org/2/library/struct.html#format-characters)

        Raise RuntimeError if invalid handshaking or broken connection.
        """
        data_recv = self.sock.recv(4)

        num = struct.unpack(">I", data_recv)[0]
        n_pts = len(data)

        if num == n_pts:
            format_ = ('%s'+format_char) % n_pts
            buff = struct.pack(format_, *data.astype(dtype))
            sent = self.sock.send(buff)

            if sent == 0:
                raise RuntimeError('Failed to send buffer. Socket connection broken.')
        else:
            raise RuntimeError('Invalid handshaking')

    # -------------------------------------------------------
    # Current session information
    # -------------------------------------------------------

    def get_stats(self):
        """ Print server statistics """
        self.send(make_command(1, 2))
        msg = self.recv_timeout('EOKS')
        print msg

    def __del__(self):
        if hasattr(self, 'sock'):
            self.sock.close()


class Commands:
    """ KServer commands

    Retrieves and stores the commands (devices and
    associated operations) available in KServer.
    """
    def __init__(self, client):
        """ Receive and parse the commands description message sent by KServer
        """
        self.success = True

        try:
            client.send(make_command(1, 1))
        except:
            if client.verbose:
                traceback.print_exc()

            print("Socket connection broken")
            self.success = False
            return

        msg = client.recv_timeout('EOC')

        if msg == "RECV_ERR_TIMEOUT":
            print("Timeout at message reception")
            self.success = False
            return

        lines = msg.split('\n')

        self.devices = []

        for line in lines[1:-2]:
            self.devices.append(DevParam(line))

        if client.verbose:
            self.print_devices()

    def get_device(self, device_name):
        """ Provide device parameters """
        for device in self.devices:
            if device.name == device_name:
                return device

        raise ValueError('Device ' + device_name + ' unknown')

    def print_devices(self):
        """ Print the devices and operations available on KServer """
        print("Devices available from KServer:")

        for device in self.devices:
            device.show()


class DevParam:
    """ Device parameters

    Store the parameters related to a devices:
    the device name and its ID, together with
    the associated operations and their reference
    """

    def __init__(self, line):
        """ Parse device informations sent by tcp-server """
        tokens = line.split(':')
        self.id = int(tokens[0][1:])
        self.name = tokens[1].strip()

        self.operations = []
        op_num = 0

        for tok in tokens[2:]:
            if len(tok.strip()) != 0:
                self.operations.append(tok.strip())
                op_num = op_num + 1

        self.op_num = op_num

    def get_op_ref(self, op_name):
        """ Return the reference of a given operation

        Args:
            op_name: Name of the operation
        """
        try:
            return self.operations.index(op_name)
        except:
            return -1

    def show(self):
        """ Display the device parameters """
        print('\n> ' + self.name)
        print('ID: ' + str(self.id))
        print('Operations:')
