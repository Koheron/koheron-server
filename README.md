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

All the public functions (except the constructor, the destructor) are made accessible through TCP and/or WebSocket. The class is statically compiled into the server to maximize performance.

Simply add the path of `hello_world.hpp` into the `devices` section of your config file (checkout the [config](config) folder for examples) and build `make CONFIG=<config_file.yml>`.

### Python TCP client

``` py
from koheron_tcp_client import KClient, command

class HelloWorld(object):
    def __init__(self, client):
        self.client = client

    @command('HELLO_WORLD','I')
    def add_42(self, num):
    	return self.client.recv_uint32()

if __name__ == "__main__":
	client = KClient('127.0.0.1')
	hw = HelloWorld(client)
	print hw.add_42(58) # 100
```

### Javascript WebSocket client

The javascript API is compatible both for browser and NodeJS use.

```coffeescript
webclient = require('koheron-websocket-client.js')
Command = webclient.Command

class HelloWorld
    constructor : (@kclient) ->
        @device = @kclient.getDevice("HELLO_WORLD")

        @cmds =
            add_42 : @device.getCmdRef( "ADD_42" )

    add42 : (num, cb) ->
    	@kclient.readUint32(Command(@device.id, @cmds.add_42, 'I', num), cb)
    	
client = new webclient.KClient('127.0.0.1', 1)

client.init( =>
    hw = new HelloWorld(client)
    hw.add42(58, (res) -> console.log res)
)
```
