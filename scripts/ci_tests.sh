#!/bin/bash
set -e

UNIXSOCK=$1
PYTHON_2=venv/py2/bin/python
PYTHON_3=venv/py3/bin/python3

# ---------------------------------------
# Build
# ---------------------------------------

# Compile kserverd
make CONFIG=config/config_local.yaml clean all
make CONFIG=config/config_armhf.yaml clean all

# Build js API
make -C apis/js/koheron-websocket-client build

# ---------------------------------------
# Tests
# ---------------------------------------

# Compile executables in local for tests
make CONFIG=config/config_local.yaml clean all

echo "== Test Python API =="
make test_python

echo "== Test Hello World =="
node apis/js/koheron-websocket-client/tests/math.js
${PYTHON_2} tests/math.py
${PYTHON_3} tests/math.py

echo "== Test Javascript API =="
make -C apis/js/koheron-websocket-client tests

echo "== Server log =="
cat server.log
