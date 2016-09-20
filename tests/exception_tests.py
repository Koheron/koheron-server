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
        return self.client.recv_uint32()

port = int(os.getenv('PYTEST_PORT', '36000'))

@pytest.mark.parametrize('port', [port])
def test_ret_type_exception(port):
    client = KoheronClient('127.0.0.1', port)
    tests = ExceptionTests(client)
    with pytest.raises(TypeError) as excinfo:
        tests.ret_type_exception()
    assert str(excinfo.value) == 'ExceptionTests::ret_type_exception returns a bool.'
