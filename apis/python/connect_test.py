import sys
import os
import pytest
import struct
import numpy as np

from koheron_tcp_client import KClient, command, write_buffer

class Tests:
    def __init__(self, client):
        self.client = client

    @command('TESTS', 'IIf?')
    def rcv_many_params(self, u1, u2, f, b):
        return self.client.recv_bool()

    @command('TESTS', 'f')
    def set_float(self, f):
        return self.client.recv_bool()

    @command('TESTS', 'd')
    def set_double(self, d):
        return self.client.recv_bool()

    @command('TESTS', 'Q')
    def set_u64(self, u):
        return self.client.recv_bool()

    @command('TESTS', 'BHI')
    def set_unsigned(self, u8, u16, u32):
        return self.client.recv_bool()

    @command('TESTS', 'bh')
    def set_signed(self, i8, i16):
        return self.client.recv_bool()

    @command('TESTS')
    def read_uint64(self):
        return self.client.recv_uint64()

    @command('TESTS')
    def read_uint(self):
        return self.client.recv_uint32()

    @command('TESTS')
    def read_int(self):
        return self.client.recv_int32()

    @command('TESTS')
    def read_float(self):
        return self.client.recv_float()

    @command('TESTS')
    def read_double(self):
        return self.client.recv_double()

    @command('TESTS')
    def send_std_array(self):
        return self.client.recv_buffer(10, data_type='float32')

    @command('TESTS')
    def send_std_vector(self):
        return self.client.recv_buffer(10, data_type='float32')

    @command('TESTS', 'I')
    def send_c_array1(self, len_):
        return self.client.recv_buffer(2*len_, data_type='float32')

    @command('TESTS')
    def send_c_array2(self):
        return self.client.recv_buffer(10, data_type='float32')

    @write_buffer('TESTS')
    def set_buffer(self, data):
        return self.client.recv_bool()

    @command('TESTS')
    def get_cstr(self):
        return self.client.recv_string()

    @command('TESTS')
    def get_tuple(self):
        return self.client.recv_tuple('Ifd?')

    @command('TESTS')
    def get_tuple2(self):
        return self.client.recv_tuple('IfQd')

    @command('TESTS')
    def get_tuple3(self):
        return self.client.recv_tuple('?ffBH')

    @command('TESTS')
    def get_tuple4(self):
        return self.client.recv_tuple('bbhh')

    @command('TESTS')
    def get_binary_tuple(self):
        buff = self.client.recv_buffer(2, data_type='uint32')
        return tuple(struct.unpack('If', buff))

# Unit tests

unixsock = os.getenv('PYTEST_UNIXSOCK','/code/kserver.sock')
print unixsock

client = KClient('127.0.0.1', 36000, verbose=False)
tests = Tests(client)

client_unix = KClient(unixsock=unixsock)
tests_unix = Tests(client_unix)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_send_many_params(tests):
    assert tests.rcv_many_params(429496729, 2048, 3.14, True)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_read_uint64(tests):
    assert tests.read_uint64() == (1 << 63)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_read_uint(tests):
    assert tests.read_uint() == 301062138

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_read_int(tests):
    assert tests.read_int() == -214748364

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_read_float(tests):
    assert abs(tests.read_float() - 3.141592) < 1E-7

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_read_double(tests):
    assert abs(tests.read_double() - 2.2250738585072009) < 1E-14

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_set_float(tests):
    assert tests.set_float(12.5)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_set_unsigned(tests):
    assert tests.set_unsigned(255, 65535, 4294967295)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_set_signed(tests):
    assert tests.set_signed(-125, -32764)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_set_double(tests):
    assert tests.set_double(1.428571428571428492127)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_set_u64(tests):
    assert tests.set_u64(2225073854759576792)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_rcv_std_vector(tests):
    array = tests.send_std_vector()
    for i in range(len(array)):
        assert array[i] == i*i*i

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_rcv_std_array(tests):
    array = tests.send_std_array()
    for i in range(len(array)):
        assert array[i] == i*i

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_rcv_c_array1(tests):
    len_ = 10
    array = tests.send_c_array1(len_)
    assert len(array) == 2*len_

    for i in range(len(array)):
        assert array[i] == 0.5 * i

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_rcv_c_array2(tests):
    array = tests.send_c_array2()

    for i in range(len(array)):
        assert array[i] == 0.25 * i

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_send_buffer(tests):
    len_ = 10
    data = np.arange(len_)**2
    assert tests.set_buffer(data)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_read_string(tests):
    assert tests.get_cstr() == 'Hello !'

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_read_tuple(tests):
    tup = tests.get_tuple()
    assert tup[0] == 501762438
    assert abs(tup[1] - 507.3858) < 5E-6
    assert abs(tup[2] - 926547.6468507200) < 1E-14
    assert tup[3]

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_read_tuple2(tests):
    tup = tests.get_tuple2()
    assert tup[0] == 2
    assert abs(tup[1] - 3.14159) < 1E-6
    assert tup[2] == 742312418498347354
    assert abs(tup[3] - 3.14159265358979323846) < 1E-14

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_read_tuple3(tests):
    tup = tests.get_tuple3()
    assert not tup[0]
    assert abs(tup[1] - 3.14159) < 1E-6
    assert abs(tup[2] - 507.3858) < 5E-6
    assert tup[3] == 42
    assert tup[4] == 6553

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_read_tuple4(tests):
    tup = tests.get_tuple4()
    assert tup[0] == -127
    assert tup[1] == 127
    assert tup[2] == -32767
    assert tup[3] == 32767

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_get_binary_tuple(tests):
    tup = tests.get_binary_tuple()
    assert tup[0] == 2
    assert abs(tup[1] - 3.14159) < 1E-6
