# koheron-server

[![Circle CI](https://circleci.com/gh/Koheron/koheron-server.svg?style=shield)](https://circleci.com/gh/Koheron/koheron-server)

## What is Koheron Server ?

Koheron Server is a high performance TCP/Websocket server for instrument control.
Each command of the server is implemented as a C++ method:

``` cpp
// hello_world.hpp

class HelloWorld
{
  public:
    uint32_t add_42(uint32_t num) {return num + 42;}
};

```

On the client side:

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
