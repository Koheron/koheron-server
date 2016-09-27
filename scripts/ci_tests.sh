#!/bin/bash
set -e

UNIXSOCK=/tmp/kserver_local.sock
PYTHON_2=tmp/koheron-python/venv/py2/bin/python
PYTHON_3=tmp/koheron-python/venv/py3/bin/python3

# ---------------------------------------
# Build
# ---------------------------------------

# Compile kserverd
# Local build must be done last for tests afterwards
make CONFIG=config/config_armhf.yaml __PYTHON=python clean all
make CONFIG=config/config_local.yaml __PYTHON=python clean all

# ---------------------------------------
# Tests
# ---------------------------------------

echo "== Test Hello World =="
make __PYTHON=python test_python
${PYTHON_2} tests/basic_math.py
${PYTHON_3} tests/basic_math.py

echo "== Server log =="
cat server.log
