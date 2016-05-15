#!/bin/bash
set -e

# ---------------------------------------
# Build
# ---------------------------------------

# Compile kserverd
make BUILD_LOCAL=True DOCKER=True CONFIG=config_local.yaml clean all
make BUILD_LOCAL=True DOCKER=True CONFIG=config_armel.yaml clean all
make BUILD_LOCAL=True DOCKER=True CONFIG=config_armhf.yaml clean all
make BUILD_LOCAL=True DOCKER=True CONFIG=config_toolchain.yaml clean all

# Compile CLI
make -C cli TARGET_HOST=local clean all
make -C cli CROSS_COMPILE=arm-linux-gnueabihf- clean all
make -C cli CROSS_COMPILE=arm-linux-gnueabi- clean all

# Compile C API Tests
make -C APIs/C/tests TARGET_HOST=local clean all
make -C APIs/C/tests TARGET_HOST=arm clean all
make -C APIs/C/tests TARGET_HOST=armhf clean all
make -C APIs/C/tests TARGET_HOST=Win32 clean all
make -C APIs/C/tests TARGET_HOST=Win64 clean all

# Build js API
make -C APIs/js/koheron-websocket-client build

# ---------------------------------------
# Tests
# ---------------------------------------

# Compile executables in local for tests
make BUILD_LOCAL=True DOCKER=True CONFIG=config_local.yaml clean all
make -C cli TARGET_HOST=local clean all
make -C APIs/C/tests TARGET_HOST=local clean all

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
APIs/C/tests/tests --unit 127.0.0.1:36000 /code/var/run/kserver.sock

echo "== Test Javascript API =="
make -C APIs/js/koheron-websocket-client tests

echo "== Test Python API =="
cd APIs/python && py.test -r connect_test.py
cd ../..

echo "== Speed tests =="
APIs/C/tests/tests --speed

echo "== Server log =="
cat server.log
