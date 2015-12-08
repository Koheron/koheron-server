# tcp-server

[![Circle CI](https://circleci.com/gh/Koheron/tcp-server.svg?style=shield)](https://circleci.com/gh/Koheron/tcp-server)

#### `High performance TCP / Websocket server for instrument control`

Koheron Server is a TCP / Websocket server optimized for high-performance instrument control applications.

### Build

To build the server call:
```
$ make CROSS_COMPILE=toolchain-
```
where `toolchain-` is the toolchain of the cross-compiler to be used.

To install a given GCC based toolchain use
```
$ sudo apt-get install gcc-toolchain
$ sudo apt-get install g++-toolchain
```

For example, to cross-compile for the Ubuntu Core distribution: 
```
$ make CROSS_COMPILE=arm-linux-gnueabihf-
```

To compile the server on the local machine use:
```
$ make TARGET_HOST=local
```

The build produces an executable called `kserverd`.

### Deploy

#### On the local machine

```
$ sudo kserverd -c kserver.conf
```

#### On a remote machine

Transfer the executable and the configuration file to the remote machine
```
$ scp kserverd root@<host_ip>:/tmp:kserverd
$ scp kserver.conf root@<host_ip>:/tmp/kserver.conf
```
where `<host_ip>` is the IP address of the remote host. Then launch the daemon: from a secure shell on the remote machine
```
# /tmp/kserverd -c /tmp/kserver.conf
```

#### Add the server to our install

Install the executable and the configuration files into a folder of your choice, for example `/usr/local/tcp-server`:
```
# mkdir /usr/local/tcp-server
# cp kserverd /usr/local/tcp-server
# cp kserver.conf /usr/local/tcp-server
```

Launch the server after initialization of the ethernet connection: add the following line into `/etc/network/interfaces.d/eth0`:
```
post-up /usr/local/tcp-server/kserverd -c /usr/local/tcp-server/kserver.conf
```

Add the same line into `/etc/network/interfaces.d/wlan0` for initialization when the Wifi is up.

