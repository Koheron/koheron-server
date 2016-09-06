# koheron-server

[![Circle CI](https://circleci.com/gh/Koheron/koheron-server.svg?style=shield)](https://circleci.com/gh/Koheron/koheron-server)

## What is Koheron Server ?

Koheron Server is a high performance TCP/Websocket server for instrument control.
Each command of the server is implemented as a C++ method:

``` cpp

class Math
{
  public:
    uint32_t add(uint32_t a, uint32_t b) {return a + b;}
};

```

On the client side:

``` py
from koheron import KoheronClient, command

class Math(object):
    def __init__(self, client):
        self.client = client

    @command()
    def add(self, num):
        return self.client.recv_uint32()

if __name__ == "__main__":
    client = KoheronClient('127.0.0.1')
    math = Math(client)
    assert math.add(1, 1) == 2
```
