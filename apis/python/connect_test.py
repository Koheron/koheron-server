import sys
import pytest
import struct
import numpy as np

from koheron_tcp_client import KClient, command, write_buffer

class Tests:
    def __init__(self, client):
        self.client = client

    @command('TESTS', 'IIf?')
    def rcv_many_params(self, u1, u2, f, b):
        return client.recv_bool()

    @command('TESTS', 'f')
    def set_float(self, f):
        return client.recv_bool()

    @command('TESTS', 'd')
    def set_double(self, d):
        return client.recv_bool()

    @command('TESTS', 'Q')
    def set_u64(self, u):
        return client.recv_bool()

    @command('TESTS')
    def read_uint64(self):
        return client.recv_uint64()

    @command('TESTS')
    def read_uint(self):
        return client.recv_uint32()

    @command('TESTS')
    def read_int(self):
        return client.recv_int32()

    @command('TESTS')
    def read_float(self):
        return client.recv_float()

    @command('TESTS')
    def read_double(self):
        return client.recv_double()

    @command('TESTS')
    def send_std_array(self):
        return client.recv_buffer(10, data_type='float32')

    @command('TESTS')
    def send_std_vector(self):
        return client.recv_buffer(10, data_type='float32')

    @command('TESTS', 'I')
    def send_c_array1(self, len_):
        return client.recv_buffer(2*len_, data_type='float32')

    @command('TESTS')
    def send_c_array2(self):
        return client.recv_buffer(10, data_type='float32')

    @write_buffer('TESTS')
    def set_buffer(self, data):
        return client.recv_bool()

    @command('TESTS')
    def get_cstr(self):
        return client.recv_string()

    @command('TESTS')
    def get_tuple(self):
        return client.recv_tuple()

    @command('TESTS')
    def get_tuple2(self):
        return client.recv_tuple('IfQ')

    @command('TESTS')
    def get_binary_tuple(self):
        buff = client.recv_buffer(2, data_type='uint32')
        return tuple(struct.unpack('If', buff))

# Unit tests

client = KClient('127.0.0.1', 36000, verbose=False)
tests = Tests(client)

@pytest.mark.parametrize('tests', [tests])
def test_send_many_params(tests):
    assert tests.rcv_many_params(429496729, 2048, 3.14, True)

@pytest.mark.parametrize('tests', [tests])
def test_read_uint64(tests):
    assert tests.read_uint64() == (1 << 63)

@pytest.mark.parametrize('tests', [tests])
def test_read_uint(tests):
    assert tests.read_uint() == 301062138

@pytest.mark.parametrize('tests', [tests])
def test_read_int(tests):
    assert tests.read_int() == -214748364

@pytest.mark.parametrize('tests', [tests])
def test_read_float(tests):
    assert abs(tests.read_float() - 3.141592) < 1E-7

@pytest.mark.parametrize('tests', [tests])
def test_read_double(tests):
    assert abs(tests.read_double() - 2.2250738585072009) < 1E-14

@pytest.mark.parametrize('tests', [tests])
def test_set_float(tests):
    assert tests.set_float(12.5)

@pytest.mark.parametrize('tests', [tests])
def test_set_double(tests):
    assert tests.set_double(1.428571428571428492127)

@pytest.mark.parametrize('tests', [tests])
def test_set_u64(tests):
    assert tests.set_u64(2225073854759576792)

@pytest.mark.parametrize('tests', [tests])
def test_rcv_std_vector(tests):
    array = tests.send_std_vector()
    for i in range(len(array)):
        assert array[i] == i*i*i

@pytest.mark.parametrize('tests', [tests])
def test_rcv_std_array(tests):
    array = tests.send_std_array()
    for i in range(len(array)):
        assert array[i] == i*i

@pytest.mark.parametrize('tests', [tests])
def test_rcv_c_array1(tests):
    len_ = 10
    array = tests.send_c_array1(len_)
    assert len(array) == 2*len_

    for i in range(len(array)):
        assert array[i] == 0.5 * i

@pytest.mark.parametrize('tests', [tests])
def test_rcv_c_array2(tests):
    array = tests.send_c_array2()

    for i in range(len(array)):
        assert array[i] == 0.25 * i

@pytest.mark.parametrize('tests', [tests])
def test_send_buffer(tests):
    len_ = 10
    data = np.arange(len_)**2
    assert tests.set_buffer(data)

@pytest.mark.parametrize('tests', [tests])
def test_read_string(tests):
    assert tests.get_cstr() == 'Hello !'

# @pytest.mark.parametrize('tests', [tests])
# def test_read_tuple(tests):
#     tup = tests.get_tuple()
#     assert tup[0] == 2
#     assert tup[1] == 3.14159
#     assert tup[2] == 2345.6

@pytest.mark.parametrize('tests', [tests])
def test_read_tuple2(tests):
    tup = tests.get_tuple2()
    assert tup[0] == 2
    assert abs(tup[1] - 3.14159) < 1E-6
    assert tup[2] == 742312418498347354

@pytest.mark.parametrize('tests', [tests])
def test_get_binary_tuple(tests):
    tup = tests.get_binary_tuple()
    assert tup[0] == 2
    assert abs(tup[1] - 3.14159) < 1E-6
