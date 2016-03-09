# tcp-server

[![Circle CI](https://circleci.com/gh/Koheron/tcp-server.svg?style=shield)](https://circleci.com/gh/Koheron/tcp-server)

#### `High performance TCP / Websocket server for instrument control`

Koheron server aims at solving the problem of interfacing hardware drivers with communication protocols. Modern instruments require a fast, low-latency control, able to cope with large data set. Moreover, the user interface ranges from heavy TCP based to light browser embedded clients. Proposing various UI to the instruments often requires implementing several protocols on the client side.

Koheron server is designed to interface C++ drivers with various communications protocols (TCP, WebSocket or UnixSockets), giving you a lot of flexibility in the way you control your instrument. Your driver is statically compiled into the server to maximize performances. When your driver is exposed via an API composed of a set of C++ classes, it is straighforward to build the reciprocal API on the client side. The client API can be in Javascript for browser-based clients using the WebSocket protocol, or in Python or C for use with the TCP or Unisocket protocols.

Before compiling the server, you need to define in the config file the set of the header files containing the classes of your API. After the build, each class will be available as a device and each public function of the class will be a command associated to the related device. From the client side you can then call each command by sending the associated function arguments. If a non void value is returned by the function, then the server will send you back the result. If required, the behavior of each command can be tuned using a simple pragma language. 

### [Building the server](doc/build.md)

