# Command Line Interface

The command line interface (CLI) is an administration tool for the Koheron server daemon.

A list of the available commands can be obtained by executing:
```
$ kserver --help
```

To start, restart and stop the server service use
```
$ systemctl start koheron-server
$ systemctl restart koheron-server
$ systemctl stop koheron-server
```

## Host

The command line is available in local via an Unix socket or remotely using TCP.

The Unix socket listens to `unix:///var/run/koheron-server.sock`.

The configuration is saved in the file `.kserver_cli` in the directory that contains the executable `kserver` of the CLI.

For an Unix socket connection run
```
$ kserver host --unix
```
and for a TCP connection use
```
$ kserver host --tcp [IP] [Port]
```
Check the current connection configuration:
```
$ kserver host --status
```

## Status

Provides information about the current status of the server.

Check whether the server is running:
```
$ kserver status
```

Available options:
```
$ kserver status --help
```

### Devices

List the available devices:
```
$ kserver status --devices
```

List the commands of a given device:
```
$ kserver status --devices [DEVICE_NAME]
```

### Sessions

Display the running sessions:
```
$ kserver status --sessions
```

## Init

This command is called at network connection post-up. It is defined in the `INIT` device. Call:
```
$ kserver init
```
