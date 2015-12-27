#!/bin/sh

# Relaunch the server if it goes down.
#
# (c) Koheron

# Duration between 2 consecutives checks
check_interval=5

# Check whether kserverd is started and start the server if it is down.
#
# https://www.digitalocean.com/community/tutorials/how-to-use-a-simple-bash-script-to-restart-server-programs
_relaunch_kserver()
{
    ps auxw | grep kserverd | grep -v grep > /dev/null

    if [ $? != 0 ]; then
        /usr/local/tcp-server/kserverd -c /usr/local/tcp-server/kserver.conf > /dev/null
    fi
}

# Check periodically
while true ; do
    _relaunch_kserver&
    sleep ${check_interval}
done
