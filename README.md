# tcp-server

[![Circle CI](https://circleci.com/gh/Koheron/tcp-server.svg?style=shield)](https://circleci.com/gh/Koheron/tcp-server)

#### `High performance TCP/Websocket server for instrument control`

Koheron tcp-server helps interfacing hardware drivers with standard communication protocols (WebSocket, TCP or UnixSockets).
Drivers are statically compiled into the server for maximum performance. 
Javascript, Python and C APIs are provided to communicate with the server.

### Interfacing a driver

Before compiling the server, you need to define in the configuration file the set of header files containing the classes of your API.
After the build, each public function of a class can be accessed from the client with a simple command.

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
    
    // The function Close() cannot be accessed from the client
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

First, we need to add the path to the file `gpio.hpp` in the configuration file of the server.
During the build the class will be integrated into the server.
All the public functions (except the constructor, the destructor and `Close` which is explicitly excluded) will be callable from each communication interface.

Once the server is [build and deployed](doc/build.md) we can interact with it.
For example, we can build a Python TCP client calling the driver functions in the following way:
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
