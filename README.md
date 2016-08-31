# koheron-server

[![Circle CI](https://circleci.com/gh/Koheron/koheron-server.svg?style=shield)](https://circleci.com/gh/Koheron/koheron-server)

#### `High performance TCP/Websocket server for instrument control`

### Accessing C++ code through the network

Consider the following C++ class

``` cpp
// hello_world.hpp

class HelloWorld
{
  public:
    uint32_t add_42(uint32_t num) {return num + 42;}
};

```

All the public functions (except the constructor, the destructor) are made accessible through TCP and/or WebSocket. The class is statically compiled into the server to maximize performance.

Simply add the path of `hello_world.hpp` into the `devices` section of your config file (checkout the [config](config) folder for examples) and build `make CONFIG=<config_file.yml>`.

### Python client

``` py
from koheron import KoheronClient, command

class HelloWorld(object):
    def __init__(self, client):
        self.client = client

    @command()
    def add_42(self, num):
        return self.client.recv_uint32()

if __name__ == "__main__":
    client = KoheronClient('127.0.0.1')
    hw = HelloWorld(client)
    print hw.add_42(58) # 100
```
