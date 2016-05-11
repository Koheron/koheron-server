import sys
import pytest

from koheron_tcp_client import KClient, command

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


# Unit tests

client = KClient('127.0.0.1', 36000, verbose=False)
tests = Tests(client)

@pytest.mark.parametrize('tests', [tests])
def test_send_many_params(tests):
    assert tests.rcv_many_params(42, 2048, 3.14, True) == True

@pytest.mark.parametrize('tests', [tests])
def test_read_uint(tests):
    assert tests.read_uint() == 42

@pytest.mark.parametrize('tests', [tests])
def test_read_int(tests):
    print tests.read_int()
    assert tests.read_int() == -42

@pytest.mark.parametrize('tests', [tests])
def test_set_float(tests):
    assert tests.set_float(12.5) == True



# print tests.send_std_array()