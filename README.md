# koheron-server

[![Circle CI](https://circleci.com/gh/Koheron/koheron-server.svg?style=svg)](https://circleci.com/gh/Koheron/koheron-server)

#### `High performance TCP / Websocket server for instrument control`

Koheron Server is a TCP / Websocket server optimized for high-performance instrument control applications.

:warning: In prototyping mode, Koheron Server allows remote (client-side) access to device memory.

### Dependencies

Cross-compiling for the Ubuntu Core distribution requires the following toolchains (`gcc version >= 4.8`):
```
$ sudo apt-get install gcc-arm-linux-gnueabihf
$ sudo apt-get install g++-arm-linux-gnueabihf
```

## Access log
```
$ cat /var/log/syslog | grep --text KServer
```

## Kill daemon
```
$ sudo pkill -SIGINT kserverd
```


