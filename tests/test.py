#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os
import pytest
import struct
import numpy as np
import re

sys.path = [".."] + sys.path
from koheron import KoheronClient, command, __version__

# http://stackoverflow.com/questions/32234169/sha1-string-regex-for-python
def is_valid_sha1(sha):
    try:
        sha_int = int(sha, 16)
    except ValueError:
        return False
    return True

class Tests:
    def __init__(self, client):
        self.client = client

    @command(classname='KServer', funcname='get_version')
    def get_server_version(self):
        return self.client.recv_string()

    @command()
    def rcv_many_params(self, u1, u2, f, b):
        return self.client.recv_bool()

    @command()
    def set_float(self, f):
        return self.client.recv_bool()

    @command()
    def set_double(self, d):
        return self.client.recv_bool()

    @command()
    def set_u64(self, u):
        return self.client.recv_bool()

    @command()
    def set_i64(self, i):
        return self.client.recv_bool()

    @command()
    def set_unsigned(self, u8, u16, u32):
        return self.client.recv_bool()

    @command()
    def set_signed(self, i8, i16, i32):
        return self.client.recv_bool()

    @command()
    def read_uint64(self):
        return self.client.recv_uint64()

    @command()
    def read_uint(self):
        return self.client.recv_uint32()

    @command()
    def read_int(self):
        return self.client.recv_int32()

    @command()
    def read_float(self):
        return self.client.recv_float()

    @command()
    def read_double(self):
        return self.client.recv_double()

    @command()
    def send_std_array(self):
        return self.client.recv_array(10, dtype='float32')

    @command()
    def send_std_array2(self, mul):
        return self.client.recv_array(512, dtype='uint32')

    @command()
    def send_std_array3(self, add):
        return self.client.recv_array(48, dtype='uint32')

    @command()
    def send_std_array4(self, add):
        return self.client.recv_array(48, dtype='uint32')

    @command()
    def send_std_vector(self):
        return self.client.recv_vector(dtype='float32')

    @command()
    def send_std_vector2(self):
        return self.client.recv_vector(dtype='uint32')

    @command()
    def send_std_vector3(self):
        return self.client.recv_vector(dtype='int32')

    @command()
    def rcv_std_array(self, u, f, arr, d, i):
        return self.client.recv_bool()

    @command()
    def rcv_std_array2(self, arr):
        return self.client.recv_bool()

    @command()
    def rcv_std_array3(self, arr):
        return self.client.recv_bool()

    @command()
    def rcv_std_array4(self, arr):
        return self.client.recv_bool()

    @command()
    def rcv_std_vector(self, vec):
        return self.client.recv_bool()

    @command()
    def rcv_std_vector1(self, u, f, vec):
        return self.client.recv_bool()

    @command()
    def rcv_std_vector2(self, u, f, vec, d, i):
        return self.client.recv_bool()

    @command()
    def rcv_std_vector3(self, arr, vec, d, i):
        return self.client.recv_bool()

    @command()
    def rcv_std_vector4(self, vec, d, i, arr):
        return self.client.recv_bool()

    @command()
    def rcv_std_vector5(self, vec1, d, i, vec2):
        return self.client.recv_bool()

    @command()
    def rcv_std_string(self, str):
        return self.client.recv_bool()

    @command()
    def rcv_std_string1(self, str):
        return self.client.recv_bool()

    @command()
    def rcv_std_string2(self, str, vec, d, i):
        return self.client.recv_bool()

    @command()
    def rcv_std_string3(self, vec, d, i, str, arr):
        return self.client.recv_bool()

    @command()
    def get_cstr(self):
        return self.client.recv_string()

    @command()
    def get_std_string(self):
        return self.client.recv_string()

    @command()
    def get_json(self):
        return self.client.recv_json()

    @command()
    def get_json2(self):
        return self.client.recv_json()

    @command()
    def get_tuple(self):
        return self.client.recv_tuple('Idd?')

    @command()
    def get_tuple2(self):
        return self.client.recv_tuple('IfQdq')

    @command()
    def get_tuple3(self):
        return self.client.recv_tuple('?ffBH')

    @command()
    def get_tuple4(self):
        return self.client.recv_tuple('bbhhii')

# Unit Tests

unixsock = os.getenv('PYTEST_UNIXSOCK','/tmp/kserver_local.sock')
port = int(os.getenv('PYTEST_PORT', '36000'))

client = KoheronClient('127.0.0.1', port)
tests = Tests(client)

client_unix = KoheronClient(unixsock=unixsock)
tests_unix = Tests(client_unix)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_get_server_version(tests):
    server_version = tests.get_server_version()
    server_version_ = server_version.split('.')
    sha = server_version_[3]
    assert len(sha) >= 7
    assert is_valid_sha1(sha)
    client_version_ = __version__.split('.')
    assert client_version_[0] >= server_version_[0]
    assert client_version_[1] >= server_version_[1]

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
    assert len(array) == 10
    for i in range(len(array)):
        assert array[i] == i*i*i

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_send_std_vector2(tests):
    array = tests.send_std_vector2()
    assert len(array) == 20
    for i in range(len(array)):
        assert array[i] == i*i

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_send_std_vector3(tests):
    array = tests.send_std_vector3()
    assert len(array) == 20
    for i in range(len(array)):
        assert array[i] == -i*i

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_send_std_array(tests):
    array = tests.send_std_array()
    for i in range(len(array)):
        assert array[i] == i*i

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_send_std_array2(tests):
    array = tests.send_std_array2(10)
    for i in range(len(array)):
        assert array[i] == 10 * i

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_send_std_array3(tests):
    array = tests.send_std_array3(5890)
    for i in range(len(array)):
        assert array[i] == 5890 + i

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_send_std_array4(tests):
    array = tests.send_std_array4(780)
    for i in range(len(array)):
        assert array[i] == 780 + 2 * i

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
def test_rcv_std_array4(tests):
    arr = np.arange(1024, dtype='uint32')
    assert tests.rcv_std_array4(arr)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_array_length_exception(tests):
    arr = np.arange(16384, dtype='float64')
    with pytest.raises(ValueError) as excinfo:
        tests.rcv_std_array3(arr)
    assert str(excinfo.value) == 'Invalid array length. Expected 8192 but received 16384.'

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_array_type_exception(tests):
    arr = np.arange(8192, dtype='float32')
    with pytest.raises(TypeError) as excinfo:
        tests.rcv_std_array3(arr)
    assert str(excinfo.value) == 'Invalid array type. Expected float64 but received float32.'

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_rcv_std_vector(tests):
    vec = np.arange(8192, dtype='uint32')
    assert tests.rcv_std_vector(vec)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_rcv_std_vector1(tests):
    vec = np.sin(np.arange(8192, dtype='float64'))
    assert tests.rcv_std_vector1(4223453, 3.141592, vec)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_rcv_std_vector2(tests):
    vec = np.log(np.arange(8192, dtype='float32') + 1)
    assert tests.rcv_std_vector2(4223453, 3.141592, vec, 2.654798454646, -56789)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_rcv_std_vector3(tests):
    arr = np.arange(8192, dtype='uint32')
    vec = np.log(np.arange(8192, dtype='float32') + 1)
    assert tests.rcv_std_vector3(arr, vec, 2.654798454646, -56789)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_rcv_std_vector4(tests):
    arr = np.arange(8192, dtype='uint32') ** 2
    vec = np.cos(np.arange(8192, dtype='float32'))
    assert tests.rcv_std_vector4(vec, 2.654798454646, -56789, arr)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_rcv_std_vector5(tests):
    vec1 = np.tanh(np.arange(8192, dtype='float32'))
    vec2 = np.log2(np.arange(16384, dtype='float32') + 1)
    assert tests.rcv_std_vector5(vec1, 0.4232747024077716, 35591508, vec2)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_rcv_string(tests):
    assert tests.rcv_std_string('Hello World')

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_rcv_string1(tests):
    assert tests.rcv_std_string1('Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer nec odio. Praesent libero. Sed cursus ante dapibus diam. Sed nisi. Nulla quis sem at nibh elementum imperdiet. Duis sagittis ipsum. Praesent mauris. Fusce nec tellus sed augue semper porta. Mauris massa. Vestibulum lacinia arcu eget nulla. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Curabitur sodales ligula in libero. Sed dignissim lacinia nunc.')

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_rcv_string2(tests):
    vec = np.sin(np.arange(8192, dtype='float32'))
    assert tests.rcv_std_string2('At vero eos et accusamus et iusto odio dignissimos ducimus, qui blanditiis praesentium voluptatum deleniti atque corrupti', vec, 0.80773675317454, -361148845)

@pytest.mark.parametrize('tests', [tests, tests_unix])
def test_rcv_string3(tests):
    vec = np.sqrt(np.arange(8192, dtype='float32'))
    arr = np.arange(8192, dtype='uint32') ** 2
    assert tests.rcv_std_string3(vec, 0.4741953746153866, -6093602, 'Erbium is a rare-earth element that, when excited, emits light around 1.54 micrometers - the low-loss wavelength for optical fibers used in DWDM. A weak signal enters the erbium-doped fiber, into which light at 980nm or 1480nm is injected using a pump laser. This injected light stimulates the erbium atoms to release their stored energy as additional 1550nm light. As this process continues down the fiber, the signal grows stronger. The spontaneous emissions in the EDFA also add noise to the signal; this determines the noise figure of an EDFA.', arr)

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
