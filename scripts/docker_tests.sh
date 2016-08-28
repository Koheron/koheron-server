#!/bin/bash
set -e

VENV=$1
UNIXSOCK=$2

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
python tests/hello_world.py
python3 tests/hello_world.py

echo "== Test Javascript API =="
make -C apis/js/koheron-websocket-client tests

echo "== Test Python API =="
python  -m pytest -v tests/tests.py
python3 -m pytest -v tests/tests.py

echo "== Server log =="
cat server.log
