import sys
import os
import pytest
import struct
import numpy as np

from koheron import KoheronClient, command, write_buffer

class Tests:
    def __init__(self, client):
        self.client = client

    @command('Tests', 'IIf?')
    def rcv_many_params(self, u1, u2, f, b):
        return self.client.recv_bool()

    @command('Tests', 'f')
    def set_float(self, f):
        return self.client.recv_bool()

    @command('Tests', 'd')
    def set_double(self, d):
        return self.client.recv_bool()

    @command('Tests', 'Q')
    def set_u64(self, u):
        return self.client.recv_bool()

    @command('Tests', 'q')
    def set_i64(self, i):
        return self.client.recv_bool()

    @command('Tests', 'BHI')
    def set_unsigned(self, u8, u16, u32):
        return self.client.recv_bool()

    @command('Tests', 'bhi')
    def set_signed(self, i8, i16, i32):
        return self.client.recv_bool()

    @command('Tests')
    def read_uint64(self):
        return self.client.recv_uint64()

    @command('Tests')
    def read_uint(self):
        return self.client.recv_uint32()

    @command('Tests')
    def read_int(self):
        return self.client.recv_int32()

    @command('Tests')
    def read_float(self):
        return self.client.recv_float()

    @command('Tests')
    def read_double(self):
        return self.client.recv_double()

    @command('Tests')
    def send_std_array(self):
        return self.client.recv_array(10, dtype='float32')

    @command('Tests')
    def send_std_vector(self):
        return self.client.recv_array(10, dtype='float32')

    @command('Tests', 'I')
    def send_c_array1(self, len_):
        return self.client.recv_array(2*len_, dtype='float32')

    @command('Tests')
    def send_c_array2(self):
        return self.client.recv_array(10, dtype='float32')

    @write_buffer('Tests')
    def set_buffer(self, data):
        return self.client.recv_bool()

    @command('Tests', 'IfAdi')
    def rcv_std_array(self, u, f, arr, d, i):
        return self.client.recv_bool()

    @command('Tests', 'A')
    def rcv_std_array2(self, arr):
        return self.client.recv_bool()

    @command('Tests', 'A')
    def rcv_std_array3(self, arr):
        return self.client.recv_bool()

    @command('Tests')
    def get_cstr(self):
        return self.client.recv_string()

    @command('Tests')
    def get_std_string(self):
        return self.client.recv_string()

    @command('Tests')
    def get_json(self):
        return self.client.recv_json()

    @command('Tests')
    def get_json2(self):
        return self.client.recv_json()

    @command('Tests')
    def get_tuple(self):
        return self.client.recv_tuple('Idd?')

    @command('Tests')
    def get_tuple2(self):
        return self.client.recv_tuple('IfQdq')

    @command('Tests')
    def get_tuple3(self):
        return self.client.recv_tuple('?ffBH')

    @command('Tests')
    def get_tuple4(self):
        return self.client.recv_tuple('bbhhii')

    @command('Tests')
    def get_binary_tuple(self):
        buff = self.client.recv_array(2, dtype='uint32')
        return tuple(struct.unpack('If', buff))

# Unit Tests

unixsock = os.getenv('PYTEST_UNIXSOCK','/code/kserver.sock')
port = int(os.getenv('PYTEST_PORT', '36000'))

client = KoheronClient('127.0.0.1', port)
tests = Tests(client)

client_unix = KoheronClient(unixsock=unixsock)
tests_unix = Tests(client_unix)

tests.set_double(1.428571428571428492127)

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
    assert tests.set_signed(-125, -32764, -2147483645)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_set_double(tests):
    assert tests.set_double(1.428571428571428492127)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_set_u64(tests):
    assert tests.set_u64(2225073854759576792)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_set_i64(tests):
    assert tests.set_i64(-9223372036854775805)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_send_std_vector(tests):
    array = tests.send_std_vector()
    for i in range(len(array)):
        assert array[i] == i*i*i

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_send_std_array(tests):
    array = tests.send_std_array()
    for i in range(len(array)):
        assert array[i] == i*i

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_send_c_array1(tests):
    len_ = 10
    array = tests.send_c_array1(len_)
    assert len(array) == 2*len_

    for i in range(len(array)):
        assert array[i] == 0.5 * i

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_send_c_array2(tests):
    array = tests.send_c_array2()

    for i in range(len(array)):
        assert array[i] == 0.25 * i

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_set_buffer(tests):
    data = np.arange(10)**2
    assert tests.set_buffer(data)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_rcv_std_array(tests):
    arr = np.arange(8192, dtype='uint32')
    assert tests.rcv_std_array(4223453, 3.141592, arr, 2.654798454646, -56789)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_rcv_std_array2(tests):
    arr = np.log(np.arange(8192, dtype='float32') + 1)
    assert tests.rcv_std_array2(arr)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_rcv_std_array3(tests):
    arr = np.sin(np.arange(8192, dtype='float64'))
    assert tests.rcv_std_array3(arr)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_get_cstring(tests):
    assert tests.get_cstr() == 'Hello !'

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_get_std_string(tests):
    assert tests.get_std_string() == 'Hello World !'

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_get_json(tests):
    data = tests.get_json()
    assert 'date' in data
    assert 'machine' in data
    assert 'time' in data
    assert 'user' in data
    assert 'version' in data
    assert data['date'] == '20/07/2016'
    assert data['machine'] == 'PC-3'
    assert data['time'] == '18:16:13'
    assert data['user'] == 'thomas'
    assert data['version'] == '0691eed'

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_get_json2(tests):
    data = tests.get_json2()
    assert 'firstName' in data
    assert 'lastName' in data
    assert 'age' in data
    assert 'phoneNumber' in data
    assert data['firstName'] == 'John'
    assert data['lastName'] == 'Smith'
    assert data['age'] == 25
    assert len(data['phoneNumber']) == 2
    assert 'number' in data['phoneNumber'][0]
    assert 'type' in data['phoneNumber'][0]
    assert 'number' in data['phoneNumber'][1]
    assert 'type' in data['phoneNumber'][1]
    assert data['phoneNumber'][0]['number'] == '212 555-1234'
    assert data['phoneNumber'][0]['type'] == 'home'
    assert data['phoneNumber'][1]['number'] == '646 555-4567'
    assert data['phoneNumber'][1]['type'] == 'fax'

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_get_tuple(tests):
    tup = tests.get_tuple()
    assert tup[0] == 501762438
    assert abs(tup[1] - 507.3858) < 5E-6
    assert abs(tup[2] - 926547.6468507200) < 1E-14
    assert tup[3]

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_get_tuple2(tests):
    tup = tests.get_tuple2()
    assert tup[0] == 2
    assert abs(tup[1] - 3.14159) < 1E-6
    assert tup[2] == 742312418498347354
    assert abs(tup[3] - 3.14159265358979323846) < 1E-14
    assert tup[4] == -9223372036854775807

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_get_tuple3(tests):
    tup = tests.get_tuple3()
    assert not tup[0]
    assert abs(tup[1] - 3.14159) < 1E-6
    assert abs(tup[2] - 507.3858) < 5E-6
    assert tup[3] == 42
    assert tup[4] == 6553

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_get_tuple4(tests):
    tup = tests.get_tuple4()
    assert tup[0] == -127
    assert tup[1] == 127
    assert tup[2] == -32767
    assert tup[3] == 32767
    assert tup[4] == -2147483647
    assert tup[5] == 2147483647

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_get_binary_tuple(tests):
    tup = tests.get_binary_tuple()
    assert tup[0] == 2
    assert abs(tup[1] - 3.14159) < 1E-6
