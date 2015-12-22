# tcp-server

[![Circle CI](https://circleci.com/gh/Koheron/tcp-server.svg?style=shield)](https://circleci.com/gh/Koheron/tcp-server)

#### `High performance TCP / Websocket server for instrument control`

Koheron Server is a TCP / Websocket server optimized for high-performance instrument control applications.

### Requirements

You first need to get the C/C++ compiler. To install a given GCC based cross-compiler toolchain run
```
$ sudo apt-get install gcc-<toolchain>
$ sudo apt-get install g++-<toolchain>
```

The build requires Python 2.7. You also the following programs:
```
$ sudo apt-get install wget
$ sudo apt-get install git

$ sudo apt-get -y install python-pip python-dev build-essential python-virtualenv
$ sudo pip install --upgrade pip
```

### Build

To build the server call:
```
$ make CONFIG=<config.yaml>
```
where `config.yaml` is the server configuration in the [config](config) folder.

For example, to compile for the Ubuntu Core distribution. After installing the `arm-linux-gnueabihf` toolchain run
```
$ make CONFIG=config_armhf.yaml
```

To compile the server on the local machine use:
```
$ make CONFIG=config_local.yaml
```

The build produces a folder `tmp`. The server executable is `tmp/server/kserverd`.

### Deploy

#### On the local machine

```
$ sudo tmp/server/kserverd -c kserver.conf
```

Check whether the daemon is launched

```
$ ps -A | grep kserverd
```

#### On a remote machine

Transfer the executable and the configuration file to the remote machine
```
$ scp tmp/server/kserverd root@<host_ip>:/tmp
$ scp config/kserver.conf root@<host_ip>:/tmp
```
where `<host_ip>` is the IP address of the remote host.

Then connect via SSH to a remote machine
```
$ ssh root@<host_ip>
``` 
and launch the daemon from a secure shell on the remote machine:
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

You can then transfer the executable to the remote machine: 
```
$ scp cli/kserver root@<host_ip>:/tmp
```
and use it:
```
<remote_host># /tmp/kserver --help
```

See also [CLI usage](doc/command_line_interface.md).
