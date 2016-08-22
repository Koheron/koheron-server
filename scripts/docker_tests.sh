#!/bin/bash
set -e

VENV=$1

if [ "${VENV}" == "venv" ]; then
	virtualenv ${VENV}
	${VENV}/bin/pip install -r requirements.txt
	PYTHON=${VENV}/bin/python
else
	PYTHON=/usr/bin/python
fi

# ---------------------------------------
# Build
# ---------------------------------------

# Compile kserverd
make CONFIG=config/config_local.yaml PYTHON=${PYTHON} clean all
make CONFIG=config/config_armhf.yaml PYTHON=${PYTHON} clean all

# Compile CLI
make CONFIG=config/config_local.yaml PYTHON=${PYTHON} clean cli
make CONFIG=config/config_armhf.yaml PYTHON=${PYTHON} clean cli

# Compile C API Tests
make -C apis/C/tests TARGET_HOST=local clean all
make -C apis/C/tests TARGET_HOST=armhf clean all
make -C apis/C/tests TARGET_HOST=Win32 clean all
make -C apis/C/tests TARGET_HOST=Win64 clean all

# Compile hello world
make -C apis/C/hello_world/ clean all

# Build js API
make -C apis/js/koheron-websocket-client build

# ---------------------------------------
# Tests
# ---------------------------------------

# Compile executables in local for tests
make clean
make CONFIG=config/config_local.yaml PYTHON=${PYTHON} all
make CONFIG=config/config_local.yaml PYTHON=${PYTHON} cli
make -C apis/C/tests TARGET_HOST=local clean all

echo "== Start server =="
nohup tmp/kserverd -c config/kserver_docker.conf > /dev/null 2> server.log &
# nohup tmp/kserverd -c config/kserver_docker.conf 0<&- &> server.log &
ps -A | grep -w "kserverd"

echo "== Test Hello World =="
apis/C/hello_world/hello_world
node apis/js/koheron-websocket-client/tests/hello_world.js
python apis/python/hello_world.py

# echo "== Test CLI =="
# CLI=apis/cli/kserver
# ${CLI} host --tcp localhost 36000
# ${CLI} host --status
# ${CLI} status --sessions
# ${CLI} status --devices
# ${CLI} status --devices KServer
# ${CLI} status --devices Tests

echo "== Test version =="
VERSION=$(${CLI} status --version)
SHA=$(git rev-parse --short HEAD)
echo ${VERSION}
echo ${SHA}
if [ "${VERSION}" != "${SHA}" ]; then
	echo Invalid version
	exit 1
fi

echo "== Test C API =="
apis/C/tests/tests --unit 127.0.0.1:36000 /code/kserver.sock

echo "== Test Javascript API =="
make -C apis/js/koheron-websocket-client tests

echo "== Test Python API =="
git clone https://github.com/Koheron/koheron-python.git
python  -m pytest -v koheron-python/test/test.py
python3 -m pytest -v koheron-python/test/test.py

echo "== Speed tests =="
apis/C/tests/tests --speed 127.0.0.1:36000 /code/kserver.sock

echo "== Server log =="
cat server.log
