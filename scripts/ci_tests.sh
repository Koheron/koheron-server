#!/bin/bash
set -e

UNIXSOCK=/tmp/kserver_local.sock
PYTHON_2=venv/py2/bin/python
PYTHON_3=venv/py3/bin/python3

# ---------------------------------------
# Build
# ---------------------------------------

# Compile kserverd
# Local build must be done last for tests afterwards
make CONFIG=config/config_armhf.yaml __PYTHON=python clean all
make CONFIG=config/config_local.yaml __PYTHON=python clean all

# Build js API
make -C apis/js/koheron-websocket-client build

# ---------------------------------------
# Tests
# ---------------------------------------

echo "== Test Python API =="
make __PYTHON=python test_python

echo "== Test Hello World =="
node apis/js/koheron-websocket-client/tests/math.js
${PYTHON_2} tests/basic_math.py
${PYTHON_3} tests/basic_math.py

echo "== Test Javascript API =="
make -C apis/js/koheron-websocket-client tests

echo "== Server log =="
cat server.log
