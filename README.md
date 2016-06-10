# tcp-server

[![Circle CI](https://circleci.com/gh/Koheron/tcp-server.svg?style=shield)](https://circleci.com/gh/Koheron/tcp-server)

#### `High performance TCP/Websocket server for instrument control`

Koheron tcp-server helps interfacing hardware drivers with standard communication protocols (WebSocket, TCP or UnixSockets).
Drivers are statically compiled into the server for maximum performance. 
Javascript, Python and C APIs are provided to communicate with the server.

### Interfacing a driver

Below is a simple example of a GPIO driver:

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

The class is statically compiled into the server. All the public functions (except the constructor, the destructor) are then accessable using any of the supported protocols.

Here is a example of a Python TCP client communicating with the GPIO driver:
``` py
from koheron_tcp_client import KClient, command

class Gpio(object):
    def __init__(self, client):
        self.client = client
        self.open_gpio()

    def open_gpio(self):
        @command('GPIO')
        def open(self): pass

        open(self)

    @command('GPIO','II')
    def set_bit(self, index, channel): pass

    @command('GPIO','II')
    def clear_bit(self, index, channel): pass

    @command('GPIO','II')
    def toggle_bit(self, index, channel): pass

    @command('GPIO','II')
    def set_as_input(self, index, channel): pass

    @command('GPIO','II')
    def set_as_output(self, index, channel): pass

if __name__ == "__main__":
	client = KClient('192.168.1.10')
	gpio = Gpio(client)
	gpio.set_as_output(0, 2)
	gpio.set_bit(0, 2)
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