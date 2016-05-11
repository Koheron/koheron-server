import sys
import pytest

from koheron_tcp_client import KClient, command

class Tests:
    def __init__(self, client):
        self.client = client

    @command('TESTS')
    def set_float(self, f):
        return client.recv_int(4) == 1

    @command('TESTS')
    def rcv_many_params(self, u1, u2, f, b):
        return client.recv_int(4) == 1

    @command('TESTS')
    def send_std_array(self):
        return client.recv_buffer(10, data_type='float32')


client = KClient('127.0.0.1', 36000, verbose=False)
# client.get_stats()

tests = Tests(client)

@pytest.mark.parametrize('tests', [tests])
def test_set_float(tests):
    assert tests.set_float(12.5) == True

@pytest.mark.parametrize('tests', [tests])
def test_rcv_many_params(tests):
    assert tests.rcv_many_params(42, 2048, 3.14, True) == True


# print tests.send_std_array()