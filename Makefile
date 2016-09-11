# Parallel build not working
# Need an extra Makefile ?
# CPUS = $(shell nproc 2> /dev/null || echo 1)
# MAKEFLAGS += --jobs=$(CPUS)

CONFIG=config/config_local.yaml
PYTHON=/usr/bin/python

SHA=`git rev-parse --short HEAD`

# Base directory for paths
BASE_DIR=.
CONFIG_PATH=$(BASE_DIR)/$(CONFIG)
__PYTHON = $(shell bash scripts/get_python.sh $(PYTHON) $(BASE_DIR))

TMP = tmp
CORE = core
MAKE_PY = scripts/make.py

CORE_HEADERS=$(shell find $(CORE) -name '*.hpp' -o -name '*.h' -o -name '*.tpp')
CORE_SRC=$(shell find $(CORE) -name '*.cpp' -o -name '*.c')
CORE_OBJ=$(subst .cpp,.o, $(addprefix $(TMP)/, $(notdir $(CORE_SRC))))
CORE_DEP=$(CORE_SRC:.cpp=.d)

ARCH_FLAGS:=$(shell $(__PYTHON) $(MAKE_PY) --arch-flags $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.arch-flags)
OPTIM_FLAGS:=$(shell $(__PYTHON) $(MAKE_PY) --optim-flags $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.optim-flags)
DEBUG_FLAGS:=$(shell $(__PYTHON) $(MAKE_PY) --debug-flags $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.debug-flags)
DEFINES:=$(shell $(__PYTHON) $(MAKE_PY) --defines $(CONFIG_PATH) $(BASE_DIR) $(SHA) && cat $(TMP)/.defines)
CROSS_COMPILE:=$(shell $(__PYTHON) $(MAKE_PY) --cross-compile $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.cross-compile)
DEVICES:=$(shell $(__PYTHON) $(MAKE_PY) --devices $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.devices)
SERVER:=$(shell $(__PYTHON) $(MAKE_PY) --server-name $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.server-name)

DEVICES_OBJ=$(addprefix $(TMP)/, $(subst .cpp,.o,$(notdir $(filter-out %.hpp,$(DEVICES)))))
_DEVICES_PATHS=$(addprefix $(BASE_DIR)/, $(sort $(dir $(DEVICES))))
# Concat paths using ':' for VPATH
# https://www.chemie.fu-berlin.de/chemnet/use/info/make/make_8.html
semicolon:=:
empty:=
space:= $(empty) $(empty)
DEVICES_PATHS=$(subst $(space),$(semicolon),$(_DEVICES_PATHS))

KS_DEVICES_HPP=$(addprefix ks_,$(notdir $(filter-out %.cpp,$(DEVICES))))
KS_DEVICES_CPP=$(addprefix $(TMP)/,$(subst .hpp,.cpp,$(KS_DEVICES_HPP)))
KS_DEVICES_OBJ=$(addprefix $(TMP)/,$(subst .hpp,.o,$(KS_DEVICES_HPP)))

DEVICE_TABLE_HPP=$(TMP)/devices_table.hpp
DEVICES_HPP=$(TMP)/devices.hpp

VPATH=core:core/crypto:$(DEVICES_PATHS)

EXECUTABLE=$(TMP)/$(SERVER)

# --------------------------------------------------------------
# Toolchains
# --------------------------------------------------------------

# Use Link Time Optimization
CC=$(CROSS_COMPILE)gcc -flto
CCXX=$(CROSS_COMPILE)g++ -flto

# --------------------------------------------------------------
# GCC compiling & linking flags
# --------------------------------------------------------------

INC=-I$(TMP) -I$(BASE_DIR)
CFLAGS=-Wall -Werror -Wno-unknown-pragmas $(INC) $(DEFINES) -MMD
CFLAGS += $(ARCH_FLAGS) $(DEBUG_FLAGS) $(OPTIM_FLAGS)
CXXFLAGS=$(CFLAGS) -std=c++14 -pthread

# Track core dependencies
# http://bruno.defraine.net/techtips/makefile-auto-dependencies-with-gcc/
-include $(CORE_DEP)

# --------------------------------------------------------------
# Libraries
# --------------------------------------------------------------

LIBS = -lm # -lpthread -lssl -lcrypto

.PHONY: all debug requirements clean start_server stop_server test_python

all: $(EXECUTABLE)

debug:
	@echo CORE_SRC = $(CORE_SRC)
	@echo CORE_OBJ = $(CORE_OBJ)
	@echo CORE_DEP = $(CORE_DEP)
	@echo DEVICES = $(DEVICES)
	@echo DEVICES_OBJ = $(DEVICES_OBJ)
	@echo DEVICES_PATHS = $(DEVICES_PATHS)
	@echo KS_DEVICES_HPP = $(KS_DEVICES_HPP)
	@echo KS_DEVICES_CPP = $(KS_DEVICES_CPP)
	@echo KS_DEVICES_OBJ = $(KS_DEVICES_OBJ)

# ------------------------------------------------------------------------------------------------------------
# Build, start, stop
# ------------------------------------------------------------------------------------------------------------

$(TMP):
	mkdir -p $(TMP)

$(DEVICE_TABLE_HPP) $(DEVICES_HPP) $(KS_DEVICES_CPP): $(TMP) $(DEVICES)
	$(__PYTHON) $(MAKE_PY) --generate $(CONFIG_PATH) $(BASE_DIR) $(TMP)

$(TMP)/%.o: %.cpp $(DEVICE_TABLE_HPP) $(DEVICES)
	$(CCXX) -c $(CXXFLAGS) -o $@ $<

$(TMP)/ks_%.o: $(TMP)/ks_%.cpp $(KS_DEVICES_CPP)
	$(CCXX) -c $(CXXFLAGS) -o $@ $<

$(EXECUTABLE): $(CORE_OBJ) $(KS_DEVICES_OBJ) $(DEVICES_OBJ)
	$(CCXX) -o $@ $(wildcard $(TMP)/*.o) $(CXXFLAGS) $(LIBS)

start_server: $(EXECUTABLE) stop_server
	nohup $(EXECUTABLE) -c config/kserver_local.conf > /dev/null 2> server.log &

stop_server:
	-pkill -SIGINT $(SERVER) # We ignore the error raised if the server is already stopped

# ------------------------------------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------------------------------------

TESTS_VENV = venv
PY2_ENV = $(TESTS_VENV)/py2
PY3_ENV = $(TESTS_VENV)/py3

$(PY2_ENV): tests/requirements.txt
	virtualenv $(PY2_ENV)
	$(PY2_ENV)/bin/pip install -r tests/requirements.txt

$(PY3_ENV): tests/requirements.txt
	virtualenv -p python3 $(PY3_ENV)
	$(PY3_ENV)/bin/pip3 install -r tests/requirements.txt

test_python: $(PY2_ENV) $(PY3_ENV) start_server
	PYTEST_UNIXSOCK=/tmp/kserver_local.sock $(PY2_ENV)/bin/python -m pytest -v tests/tests.py
	PYTEST_UNIXSOCK=/tmp/kserver_local.sock $(PY3_ENV)/bin/python3 -m pytest -v tests/tests.py

# ------------------------------------------------------------------------------------------------------------
# Clean
# ------------------------------------------------------------------------------------------------------------

clean_venv:
	rm -rf $(TESTS_VENV)

clean:
	rm -rf $(TMP)
