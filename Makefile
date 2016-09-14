CONFIG=config/config_local.yaml
PYTHON=/usr/bin/python

SHA=`git rev-parse --short HEAD`

# Base directory for paths
BASE_DIR=.
CONFIG_PATH=$(BASE_DIR)/$(CONFIG)
__PYTHON = $(shell bash scripts/get_python.sh $(PYTHON) $(BASE_DIR))

TMP = $(BASE_DIR)/tmp
CORE = core
MAKE_PY = scripts/make.py

CORE_HEADERS=$(shell find $(CORE) -name '*.hpp' -o -name '*.h' -o -name '*.tpp')
CORE_SRC=$(shell find $(CORE) -name '*.cpp' -o -name '*.c')
CORE_OBJ=$(subst .cpp,.o, $(addprefix $(TMP)/, $(notdir $(CORE_SRC))))
CORE_DEP=$(subst .o,.d,$(CORE_OBJ))

ARCH_FLAGS:=$(shell $(__PYTHON) $(MAKE_PY) --arch-flags $(CONFIG_PATH) $(BASE_DIR) $(TMP) && cat $(TMP)/.arch-flags)
OPTIM_FLAGS:=$(shell $(__PYTHON) $(MAKE_PY) --optim-flags $(CONFIG_PATH) $(BASE_DIR) $(TMP) && cat $(TMP)/.optim-flags)
DEBUG_FLAGS:=$(shell $(__PYTHON) $(MAKE_PY) --debug-flags $(CONFIG_PATH) $(BASE_DIR) $(TMP) && cat $(TMP)/.debug-flags)
DEFINES:=$(shell $(__PYTHON) $(MAKE_PY) --defines $(CONFIG_PATH) $(BASE_DIR) $(TMP) $(SHA) && cat $(TMP)/.defines)
CROSS_COMPILE:=$(shell $(__PYTHON) $(MAKE_PY) --cross-compile $(CONFIG_PATH) $(BASE_DIR) $(TMP) && cat $(TMP)/.cross-compile)
DEVICES:=$(shell $(__PYTHON) $(MAKE_PY) --devices $(CONFIG_PATH) $(BASE_DIR) $(TMP) && cat $(TMP)/.devices)
DEPENDENCIES:=$(shell $(__PYTHON) $(MAKE_PY) --dependencies $(CONFIG_PATH) $(BASE_DIR) $(TMP) && cat $(TMP)/.dependencies)
SERVER:=$(shell $(__PYTHON) $(MAKE_PY) --server-name $(CONFIG_PATH) $(BASE_DIR) $(TMP) && cat $(TMP)/.server-name)

DEVICES_HPP=$(filter-out %.cpp,$(DEVICES))
DEVICES_CPP=$(filter-out %.hpp,$(DEVICES))
DEVICES_OBJ=$(addprefix $(TMP)/, $(subst .cpp,.o,$(notdir $(filter-out %.hpp,$(DEVICES)))))

DEPENDENCIES_OBJ=$(addprefix $(TMP)/, $(notdir $(subst .cpp,.o,$(DEPENDENCIES))))

_DEVICES_PATHS=$(sort $(dir $(DEVICES))) $(sort $(dir $(DEPENDENCIES)))
# Concat paths using ':' for VPATH
# https://www.chemie.fu-berlin.de/chemnet/use/info/make/make_8.html
semicolon:=:
empty:=
space:= $(empty) $(empty)
DEVICES_PATHS=$(subst $(space),$(semicolon),$(_DEVICES_PATHS))

# Generated source files
KS_DEVICES_HPP=$(addprefix ks_,$(notdir $(DEVICES_HPP)))
KS_DEVICES_CPP=$(addprefix $(TMP)/,$(subst .hpp,.cpp,$(KS_DEVICES_HPP)))
KS_DEVICES_OBJ=$(addprefix $(TMP)/,$(subst .hpp,.o,$(KS_DEVICES_HPP)))
TMP_DEVICE_TABLE_HPP=$(TMP)/devices_table.hpp
TMP_DEVICES_HPP=$(TMP)/devices.hpp

VPATH=core:core/crypto:$(DEVICES_PATHS)
OBJ = $(CORE_OBJ) $(KS_DEVICES_OBJ) $(DEVICES_OBJ) $(DEPENDENCIES_OBJ)
EXECUTABLE=$(TMP)/$(SERVER)

CPUS = $(shell nproc 2> /dev/null || echo 1)

# --------------------------------------------------------------
# Toolchains
# --------------------------------------------------------------

# Use Link Time Optimization
CC=$(CROSS_COMPILE)gcc -flto
CCXX=$(CROSS_COMPILE)g++ -flto

# --------------------------------------------------------------
# GCC compiling & linking flags
# --------------------------------------------------------------

INC=-I$(TMP) -I$(BASE_DIR) -I.
CFLAGS=-Wall -Werror $(INC) $(DEFINES) -MMD -MP
CFLAGS += $(ARCH_FLAGS) $(DEBUG_FLAGS) $(OPTIM_FLAGS)
CXXFLAGS=$(CFLAGS) -std=c++14 -pthread

# --------------------------------------------------------------
# Libraries
# --------------------------------------------------------------

LIBS = -lm # -lpthread -lssl -lcrypto

.PHONY: all debug

all: exec

debug:
	@echo CORE_SRC = $(CORE_SRC)
	@echo CORE_OBJ = $(CORE_OBJ)
	@echo CORE_DEP = $(CORE_DEP)
	@echo DEVICES_HPP = $(DEVICES_HPP)
	@echo DEVICES_CPP = $(DEVICES_CPP)
	@echo DEVICES_OBJ = $(DEVICES_OBJ)
	@echo DEPENDENCIES = $(DEPENDENCIES)
	@echo DEPENDENCIES_OBJ = $(DEPENDENCIES_OBJ)
	@echo DEVICES_PATHS = $(DEVICES_PATHS)
	@echo KS_DEVICES_HPP = $(KS_DEVICES_HPP)
	@echo KS_DEVICES_CPP = $(KS_DEVICES_CPP)
	@echo KS_DEVICES_OBJ = $(KS_DEVICES_OBJ)

# ------------------------------------------------------------------------------------------------------------
# Build, start, stop
# ------------------------------------------------------------------------------------------------------------

.PHONY: exec start_server stop_server

# Track core dependencies
# http://bruno.defraine.net/techtips/makefile-auto-dependencies-with-gcc/
# http://scottmcpeak.com/autodepend/autodepend.html
-include $(CORE_DEP)

$(TMP_DEVICE_TABLE_HPP) $(TMP_DEVICES_HPP) $(KS_DEVICES_CPP): $(DEVICES_HPP)
	$(__PYTHON) $(MAKE_PY) --generate $(CONFIG_PATH) $(BASE_DIR) $(TMP)

$(TMP)/%.o: %.cpp
	$(CCXX) -c $(CXXFLAGS) -o $@ $<

$(TMP)/ks_%.o: $(TMP)/ks_%.cpp
	$(CCXX) -c $(CXXFLAGS) -o $@ $<

$(EXECUTABLE): $(OBJ)
	$(CCXX) -o $@ $(OBJ) $(CXXFLAGS) $(LIBS)

exec: $(TMP_DEVICE_TABLE_HPP) $(TMP_DEVICES_HPP) $(KS_DEVICES_CPP)
	$(MAKE) --jobs=$(CPUS) $(EXECUTABLE)

start_server: exec stop_server
	nohup $(EXECUTABLE) -c config/kserver_local.conf > /dev/null 2> server.log &

stop_server:
	-pkill -SIGINT $(SERVER) # We ignore the error raised if the server is already stopped

# ------------------------------------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------------------------------------

.PHONY: test_python

TESTS_VENV = venv
PY2_ENV = $(TESTS_VENV)/py2
PY3_ENV = $(TESTS_VENV)/py3

$(PY2_ENV): tests/requirements.txt
	test -d $(PY2_ENV) || (virtualenv $(PY2_ENV) && $(PY2_ENV)/bin/pip install -r tests/requirements.txt)

$(PY3_ENV): tests/requirements.txt
	test -d $(PY2_ENV) || (virtualenv -p python3 $(PY3_ENV) && $(PY3_ENV)/bin/pip3 install -r tests/requirements.txt)

test_python: $(PY2_ENV) $(PY3_ENV) start_server
	PYTEST_UNIXSOCK=/tmp/kserver_local.sock $(PY2_ENV)/bin/python -m pytest -v tests/tests.py
	PYTEST_UNIXSOCK=/tmp/kserver_local.sock $(PY3_ENV)/bin/python3 -m pytest -v tests/tests.py

# ------------------------------------------------------------------------------------------------------------
# Clean
# ------------------------------------------------------------------------------------------------------------

.PHONY: clean clean_venv

clean_venv:
	rm -rf $(TESTS_VENV)

clean:
	rm -rf $(TMP)
