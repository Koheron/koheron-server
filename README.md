# tcp-server

[![Circle CI](https://circleci.com/gh/Koheron/tcp-server.svg?style=shield)](https://circleci.com/gh/Koheron/tcp-server)

#### `High performance TCP/Websocket server for instrument control`

### Accessing C++ code through the network

Consider the following C++ class

``` cpp
// hello_world.hpp
#ifndef __HELLO_WORLD_HPP__
#define __HELLO_WORLD_HPP__

class HelloWorld
{
  public:
    uint32_t add_42(uint_32_t num) {return num + 42;}
};

#endif // __HELLO_WORLD_HPP__
```

Tcp-server allows us to access all the public functions (except the constructor, the destructor) through various protocols (WebSocket, TCP or UnixSockets are supported for now). To maximize performance the class is statically compiled into the server.

Simply add the path of `hello_world.hpp` into the `devices` section of your config file (checkout the [config](config) folder for examples).

### Python client

``` py
from koheron_tcp_client import KClient, command

class HelloWorld(object):
    def __init__(self, client):
        self.client = client

    @command('HELLO_WORLD','I')
    def add_42(self, num):
    	return self.client.recv_uint32()

if __name__ == "__main__":
	client = KClient('192.168.1.10')
	hw = HelloWorld(client)
	print hw.add_42(58) # 100
```

Check our [drivers folder](https://github.com/Koheron/zynq-sdk/tree/master/drivers) to find several complete examples.

### Requirements and build

You first need to get the C/C++ compiler. To install a given GCC based cross-compiler toolchain run
```
$ sudo apt-get install gcc-<toolchain>
$ sudo apt-get install g++-<toolchain>
```

The build requires Python 2.7. You also need the following programs:
```
$ sudo apt-get install wget git
$ sudo bash scripts/install_eigen.sh

$ sudo apt-get -y install python-pip python-dev build-essential python-virtualenv libyaml-dev
$ sudo pip install --upgrade pip
$ sudo pip install -r requirements.txt
```

To build simply do
```
$ make CONFIG=<config.yaml>
```
for examples of configurqtion files checkout the [config](config) folder.
