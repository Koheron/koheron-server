#!/bin/bash
set -e

# ---------------------------------------
# Build
# ---------------------------------------

# Compile kserverd
make CONFIG=config/config_local.yaml clean all
make CONFIG=config/config_armel.yaml clean all
make CONFIG=config/config_armhf.yaml clean all

# Compile CLI
make CONFIG=config/config_local.yaml clean cli
make CONFIG=config/config_armel.yaml clean cli
make CONFIG=config/config_armhf.yaml clean cli

# Compile C API Tests
make -C apis/C/tests TARGET_HOST=local clean all
make -C apis/C/tests TARGET_HOST=arm clean all
make -C apis/C/tests TARGET_HOST=armhf clean all
make -C apis/C/tests TARGET_HOST=Win32 clean all
make -C apis/C/tests TARGET_HOST=Win64 clean all

# Build js API
make -C apis/js/koheron-websocket-client build

# ---------------------------------------
# Tests
# ---------------------------------------

# Compile executables in local for tests
make clean
make CONFIG=config/config_local.yaml all
make CONFIG=config/config_local.yaml cli
make -C apis/C/tests TARGET_HOST=local clean all

echo "== Start server =="
nohup tmp/kserverd -c config/kserver_docker.conf > /dev/null 2>server.log &
ps -A | grep -w "kserverd"

echo "== Test CLI =="
CLI=apis/cli/kserver
${CLI} host --tcp localhost 36000
${CLI} host --status
${CLI} status --sessions
${CLI} status --devices
${CLI} status --devices KSERVER
${CLI} status --devices TESTS

echo "== Test C API =="
apis/C/tests/tests --unit 127.0.0.1:36000 /code/kserver.sock

echo "== Test Javascript API =="
make -C apis/js/koheron-websocket-client tests

echo "== Test Python API =="
cd apis/python && py.test -r connect_test.py
cd ../..

echo "== Speed tests =="
apis/C/tests/tests --speed 127.0.0.1:36000 /code/kserver.sock

echo "== Server log =="
cat server.log
