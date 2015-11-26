# Command Line Interface

## Overview

The command line interface provides administration tools for the Kserver daemon. 
It can be used locally with a Unix socket or remotely via a TCP connection.

## Shell autocompletion

The file ```kserver-completion.bash``` provides autocompletion functionnalities for the CLI. It should be source:
```sh
source kserver-completion.bash
```

For integration into Ubuntu the last line should be:
```sh
complete -F _kserver kserver
```
instead of
```sh
complete -F _kserver ./kserver
```
and the file should be placed in the folder ```/etc/bash-completion.d```
