# Exception tests
# A new session is opened and closed for each test since
# they missread the reception buffer.

import os
import pytest

from koheron import KoheronClient, command

class ExceptionTests:
    def __init__(self, client):
        self.client = client

    @command()
    def ret_type_exception(self):
        return self.client.recv_uint32() # Instead of bool

    @command()
    def std_vector_exception(self):
        return self.client.recv_vector(dtype='float32') # Instead of uint32

    @command()
    def std_array_type_exception(self):
        return self.client.recv_array(shape=10, dtype='uint32') # Instead of float

    @command()
    def std_array_size_exception(self):
        return self.client.recv_array(shape=20, dtype='float32') # Instead of 10

port = int(os.getenv('PYTEST_PORT', '36000'))

@pytest.mark.parametrize('port', [port])
def test_ret_type_exception(port):
    client = KoheronClient('127.0.0.1', port)
    tests = ExceptionTests(client)
    with pytest.raises(TypeError) as excinfo:
        tests.ret_type_exception()
    assert str(excinfo.value) == 'ExceptionTests::ret_type_exception returns a bool.'

@pytest.mark.parametrize('port', [port])
def test_std_vector_exception(port):
    client = KoheronClient('127.0.0.1', port)
    tests = ExceptionTests(client)
    with pytest.raises(TypeError) as excinfo:
        tests.std_vector_exception()
    assert str(excinfo.value) == 'ExceptionTests::std_vector_exception expects elements of type uint32_t.'

@pytest.mark.parametrize('port', [port])
def test_std_array_type_exception(port):
    client = KoheronClient('127.0.0.1', port)
    tests = ExceptionTests(client)
    with pytest.raises(TypeError) as excinfo:
        tests.std_array_type_exception()
    assert str(excinfo.value) == 'ExceptionTests::std_array_type_exception expects elements of type float.'

@pytest.mark.parametrize('port', [port])
def test_std_array_size_exception(port):
    client = KoheronClient('127.0.0.1', port)
    tests = ExceptionTests(client)
    with pytest.raises(ValueError) as excinfo:
        tests.std_array_size_exception()
    assert str(excinfo.value) == 'ExceptionTests::std_array_size_exception expects 10 elements.'
