# tcp-server

[![Circle CI](https://circleci.com/gh/Koheron/tcp-server.svg?style=shield)](https://circleci.com/gh/Koheron/tcp-server)

#### `High performance TCP/Websocket server for instrument control`

Koheron server aims at solving the problem of interfacing hardware drivers with communication protocols. Modern instruments require a fast, low-latency control, able to cope with large data set. Moreover, the user interface ranges from heavy TCP based to light browser embedded clients. Proposing various UI to the instruments often requires implementing several protocols on the client side.

Koheron server is designed to interface C++ drivers with various communications protocols (TCP, WebSocket or UnixSockets), giving you a lot of flexibility in the way you control your instrument. Your driver is statically compiled into the server to maximize performances. When your driver is exposed via an API composed of a set of C++ classes, it is straighforward to build the reciprocal API on the client side. The client API can be in Javascript for browser-based clients using the WebSocket protocol, or in Python or C for use with the TCP or Unisocket protocols.

Before compiling the server, you need to define in the config file the set of the header files containing the classes of your API. After the build, each class will be available as a device and each public function of the class will be a command associated to the related device. From the client side you can then call each command by sending the associated function arguments. If a non void value is returned by the function, then the server will send you back the result. If required, the behavior of each command can be tuned using a simple pragma language.

### Interfacing a driver

Let say we would like to interface the following GPIO driver:
``` cpp
// gpio.hpp
#ifndef __GPIO_HPP__
#define __GPIO_HPP__

#include <drivers/dev_mem.hpp>
#include <drivers/wr_register.hpp>

class Gpio
{
  public:
    Gpio(Klib::DevMem& dev_mem_);
    ~Gpio();

    int Open();
    
    # pragma tcp-server exclude
    void Close();

    void set_bit(uint32_t index, uint32_t channel);
    void clear_bit(uint32_t index, uint32_t channel);
    void toggle_bit(uint32_t index, uint32_t channel);
    void set_as_input(uint32_t index, uint32_t channel);
    void set_as_output(uint32_t index, uint32_t channel);

  private:
    Klib::DevMem& dev_mem;
    Klib::MemMapID gpio_mem; // GPIO memory map ID
};

#endif // __GPIO_HPP__
```

First we need to add the path to the file `gpio.hpp` in the configuration file of the server. During the build the class will be integrated into the server. All the public functions (except the constructor, the destructor and `Close` which is explicitly excluded) will be callable from the various communication interfaces.

Once the server is [build and deployed](doc/build.md) we can interact with it. For example, we can build a Python TCP client calling the driver functions in the following way:
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

    @command('GPIO')
    def set_bit(self, index, channel): pass

    @command('GPIO')
    def clear_bit(self, index, channel): pass

    @command('GPIO')
    def toggle_bit(self, index, channel): pass

    @command('GPIO')
    def set_as_input(self, index, channel): pass

    @command('GPIO')
    def set_as_output(self, index, channel): pass

if __name__ == "__main__":
	client = KClient('192.168.1.10')
	gpio = Gpio(client)
	gpio.set_as_output(0, 2)
	gpio.set_bit(0, 2)
```

Check our [drivers folder](https://github.com/Koheron/zynq-sdk/tree/master/devices) to find several complete examples.