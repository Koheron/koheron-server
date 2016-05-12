#!/bin/bash
set -e

echo "== Start server =="
nohup tmp/server/kserverd -c config/kserver_docker.conf > /dev/null 2>server.log &
ps -A | grep -w "kserverd"

echo "== Test CLI =="
cli/kserver host --tcp localhost 36000
cli/kserver host --status
cli/kserver status --sessions
cli/kserver status --devices
cli/kserver status --devices KSERVER
cli/kserver status --devices TESTS

echo "== Test C API =="
APIs/C/tests/tests

echo "== Test Javascript API =="
make -C APIs/js/koheron-websocket-client tests

echo "== Test Python API =="
cd APIs/python && py.test -r connect_test.py
cd ../..

echo "== Server log =="
cat server.log