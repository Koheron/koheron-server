# tcp-server

[![Circle CI](https://circleci.com/gh/Koheron/tcp-server.svg?style=shield)](https://circleci.com/gh/Koheron/tcp-server)

#### `High performance TCP / Websocket server for instrument control`

Koheron Server is a TCP / Websocket server optimized for high-performance instrument control applications.

### Requirements

To install a given GCC based cross-compiler toolchain run
```
$ sudo apt-get install gcc-<toolchain>
$ sudo apt-get install g++-<toolchain>
```

### Build

To build the server call:
```
$ make CROSS_COMPILE=<toolchain>-
```

For example, to cross-compile for the Ubuntu Core distribution: 
```
$ make CROSS_COMPILE=arm-linux-gnueabihf-
```

Or to compile the server on the local machine use:
```
$ make TARGET_HOST=local
```

The build produces an executable called `kserverd`.

### Deploy

#### On the local machine

```
$ sudo ./kserverd -c kserver.conf
```

Check whether the daemon is launched

```
$ ps -A | grep kserverd
```

#### On a remote machine

Connect via SSH to a remote machine
```
$ ssh root@<host_ip>
```
where `<host_ip>` is the IP address of the remote host.

Transfer the executable and the configuration file to the remote machine
```
$ scp kserverd root@<host_ip>:/tmp
$ scp kserver.conf root@<host_ip>:/tmp
```

Then launch the daemon from a secure shell on the remote machine:
```
<remote_host># /tmp/kserverd -c /tmp/kserver.conf
```

If the server is already launched, you can end it nicely before by running:
```
<remote_host># pkill -SIGINT kserverd
```

#### Add the server to your Linux install

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

### Command line interface

To compile the CLI run
```
$ make -C cli CROSS_COMPILE=<toolchain>-
```
it generates an executable `cli/kserver`.

Transfer CLI to remote machine 
```
$ scp cli/kserver root@<host_ip>
```

See also [CLI usage](doc/command_line_interface.md).
