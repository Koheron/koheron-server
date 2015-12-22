# Command line Interface

The command line interface (CLI) is an administration tool for the Koheron server daemon.

For now, it mainly implements monitoring features. Capabilities will be expanded as we neeed them in the future.

A list of the available commands can be obtained by executing:
```
$ kserver --help
```

Note that, beyond the CLI capabilities, signals can be send directly to the daemon. To nicely stop the daemon run:
```
$ pkill -SIGINT kserverd
```

## Host

The command line is available in local via an Unix socket or remotely using TCP.

The Unix socket listens to `unix:///var/run/kserver.sock`.

The configuration is saved in the file `.kserver_cli` in the directory that contains the executable `kserver` of the CLI.

To select the Unix socket connection run:
```
$ kserver host --unix
```
Whereas the TCP connection is activated by:
```
$ kserver host --tcp [IP] [Port]
```
To check the current configuration use:
```
$ kserver host --status
```

## Status

Provides information about the current status of the server.

To obtain the available options, run:
```
$ kserver status --help
```

### Devices

To obtain the list of the available devices:
```
$ kserver status --devices
```

The commands of a given device:
```
$ kserver status --devices [DEVICE_NAME]
```

### Sessions

To display the running sessions:
```
$ kserver status --sessions
```

## Init tasks

This set of commands is designed to be called in the init scripts at system launch. They interface the `INIT_TASKS` device.

To show the last digit of the IP address on the board LEDs use:
```
$ kserver init_tasks --ip_on_leds [LEDS_ADDRESS]
```
where `LEDS_ADDRESS` the LEDs base address can be specified in hexadecimal form (e.g. `0x60000000`).
