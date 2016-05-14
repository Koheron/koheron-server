# tcp-server

[![Circle CI](https://circleci.com/gh/Koheron/tcp-server.svg?style=shield)](https://circleci.com/gh/Koheron/tcp-server)

#### `High performance TCP/Websocket server for instrument control`

Koheron tcp-server helps interfacing hardware drivers with standard communication protocols (WebSocket, TCP or UnixSockets).
Drivers are statically compiled into the server for maximum performance. 
Javascript, Python and C APIs are provided to communicate with the server.

### Interfacing a driver

Below is a simple example of a GPIO driver:

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

The class is integrated into the server during [build](doc/build.md).
All the public functions (except the constructor, the destructor and `Close` which is explicitly excluded) can be accessed using any of the supported protocols.

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

Check our [drivers folder](https://github.com/Koheron/zynq-sdk/tree/master/devices) to find several complete examples.
