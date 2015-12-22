# Kmake

## Introduction

Kmake is a Python based building utility for KServer. It handles:
* the code generation
* the compilation

## In a nutshell

### Compile KServer

1-  Edit the configuration file `config.yaml`.

2- Compile:
```
$ python kmake.py kserver -c config.yaml
```

### Add a command to a device

Here are the steps to add a new command to the device `DEV`.

1-  Edit the YAML description file of the device (`devices/dev.yaml`) to add the command using the [D2L](D2L.md).

2-  Update the fragments:
```
$ python kmake.py fragments -t devices/dev.yaml
```
then open the file `devices/ks_dev.frag` and edit the fragment related to the new command.

3- Compile KServer and that's it !

### Create a new device

> You should consider using directly the [*device interface tags*](device_interface_language.md) in the middleware header file. It will performs all these steps automatically. However this is suitable for more advanced features implementation since the fragment code is manually edited.

Mostly the same steps that to add a new command. We assume we want to create the device `DEV`.

1-  Create a new YAML file `devices/dev.yaml` for your device using the [D2L](D2L.md).

2-  Create the fragments file:
```
$ python kmake.py fragments -t devices/dev.yaml
```
then open the file `devices/ks_dev.frag` and edit the fragments for each command.

3-  Add the device to your configuration `config.yaml`: In the field `path` add `- devices/dev.yaml`.

4-  Compile KServer and you're done !

## Detailed reference

```
$ python kmake.py <command> [<args>]
```

The available commands are:
* `kserver` Build KServer: sources generation and compilation
* `fragments` Code fragments manipulations

Use
```
$ python kmake.py <command> --help
```
for the detailed usage of a given command

### Build KServer

```
$ python kmake.py kserver [-h|--help] [<options> [cfg_file]]
```
where `cfg_file` is the configuration file `config.yaml`.

The options are:
* `-g, --generate` Generate the source files
* `-c, --generate-compile` Generate the sources and compile KServer

The compilation flow for KServer is:

<img src="https://cloud.githubusercontent.com/assets/7926718/11366496/81fb32e0-92a5-11e5-97e4-1c511f648e5f.png" width="500px" />

### Manipulate code fragments

```
$ python kmake.py fragments [-h|--help] [<options> [yaml_file]]
```

The options are:
* `-t, --template` Generate a template file to edit the C++ fragments
