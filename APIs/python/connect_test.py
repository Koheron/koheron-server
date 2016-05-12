import sys
import pytest
import numpy as np

from koheron_tcp_client import KClient, command, write_buffer

class Tests:
    def __init__(self, client):
        self.client = client

    @command('TESTS', 'uufb')
    def rcv_many_params(self, u1, u2, f, b):
        return client.recv_bool()

    @command('TESTS', 'f')
    def set_float(self, f):
        return client.recv_bool()

    @command('TESTS')
    def read_uint(self):
        return client.recv_uint32()

    @command('TESTS')
    def read_int(self):
        return client.recv_int32()

    @command('TESTS')
    def send_std_array(self):
        return client.recv_buffer(10, data_type='float32')

    @command('TESTS')
    def send_std_vector(self):
        return client.recv_buffer(10, data_type='float32')

    @command('TESTS', 'u')
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

# Unit tests

client = KClient('127.0.0.1', 36000, verbose=False)
tests = Tests(client)

@pytest.mark.parametrize('tests', [tests])
def test_send_many_params(tests):
    assert tests.rcv_many_params(429496729, 2048, 3.14, True)

@pytest.mark.parametrize('tests', [tests])
def test_read_uint(tests):
    assert tests.read_uint() == 301062138

@pytest.mark.parametrize('tests', [tests])
def test_read_int(tests):
    print tests.read_int()
    assert tests.read_int() == -214748364

@pytest.mark.parametrize('tests', [tests])
def test_set_float(tests):
    assert tests.set_float(12.5)

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
    assert tests.get_cstr() == 'Helo !'

@pytest.mark.parametrize('tests', [tests])
def test_read_tuple(tests):
    tup = tests.get_tuple()
    assert tup[0] == 2
    assert tup[1] == 3.14159
    assert tup[2] == 2345.6
