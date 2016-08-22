# koheron-server

[![Circle CI](https://circleci.com/gh/Koheron/koheron-server.svg?style=shield)](https://circleci.com/gh/Koheron/koheron-server)

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
    unsigned int add_42(unsigned int num) {return num + 42;}
};

#endif // __HELLO_WORLD_HPP__
```

All the public functions (except the constructor, the destructor) are made accessible through TCP and/or WebSocket. The class is statically compiled into the server to maximize performance.

Simply add the path of `hello_world.hpp` into the `devices` section of your config file (checkout the [config](config) folder for examples) and build `make CONFIG=<config_file.yml>`.

### Python TCP client

``` py
from koheron import KoheronClient, command

class HelloWorld(object):
    def __init__(self, client):
        self.client = client

    @command('HELLO_WORLD','I')
    def add_42(self, num):
        return self.client.recv_uint32()

if __name__ == "__main__":
    client = KoheronClient('127.0.0.1')
    hw = HelloWorld(client)
    print hw.add_42(58) # 100
```

### Javascript WebSocket client

The javascript API is compatible both for browser and NodeJS use.

```coffee
webclient = require('koheron-websocket-client.js')
Command = webclient.Command

class HelloWorld
    constructor : (@kclient) ->
        @device = @kclient.getDevice("HELLO_WORLD")
        @cmds = @device.getCmds()

    add42 : (num, cb) ->
        @kclient.readUint32(Command(@device.id, @cmds.add_42, 'I', num), cb)
        
client = new webclient.KClient('127.0.0.1', 1)

client.init( =>
    hw = new HelloWorld(client)
    hw.add42(58, (res) ->
        console.log res
        process.exit()
    )
)
```

### C TCP client

```c
#include <kclient.h>
#include <stdio.h>

int main(void)
{
    struct kclient *kcl = kclient_connect("127.0.0.1", 36000);
    dev_id_t id = get_device_id(kcl, "HELLO_WORLD");
    op_id_t add_42_ref = get_op_id(kcl, id, "ADD_42");

    uint32_t res;
    kclient_send_command(kcl, id, add_42_ref, "I", 58);
    kclient_read_u32(kcl, &res);

    printf("%u\n", res);
    return 0;
}
```
