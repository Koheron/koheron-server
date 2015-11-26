# Syslog

Messages from KServer are sent to the syslog daemon of the system so that they can be catched and analysed by any system administration tool.

To easily access KServer log:
```
$ cat /var/log/syslog | grep --text KServer
```
The `--text` option is necessary because sometimes the syslog file contains some binary data.

## configuration

On Ubuntu the syslog configuration can be edited in `/etc/rsyslog.d/50-default.conf`.

See http://linux.die.net/man/5/syslog.conf for help.

After edition, you need to restart `rsyslog`:
```
$ sudo service rsyslog restart
```

The logging results can be found in `/var/log/syslog`. Note that depending on your configuration of `rsyslog` the update of the file is not necessarily synced with the program for performance reasons.
