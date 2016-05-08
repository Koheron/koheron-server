# /bin/bash

echo "== Start server =="
tmp/server/kserverd -c config/kserver_docker.conf&
ps -A

echo "== Test CLI =="
cli/kserver host --tcp localhost 36000
cli/kserver host --status
cli/kserver status --sessions
cli/kserver status --devices
cli/kserver status --devices KSERVER
cli/kserver status --devices TESTS

echo "== Test C API =="
APIs/C/tests/tests

echo "== Test Python API =="
cd APIs/python && python connect_test.py