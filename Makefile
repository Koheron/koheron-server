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
TMP_CORE_OBJ=$(subst .cpp,.o, $(addprefix $(TMP)/, $(notdir $(CORE_SRC))))

ARCH_FLAGS:=$(shell $(__PYTHON) $(MAKE_PY) --arch-flags $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.arch-flags)
OPTIM_FLAGS:=$(shell $(__PYTHON) $(MAKE_PY) --optim-flags $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.optim-flags)
DEBUG_FLAGS:=$(shell $(__PYTHON) $(MAKE_PY) --debug-flags $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.debug-flags)
DEFINES:=$(shell $(__PYTHON) $(MAKE_PY) --defines $(CONFIG_PATH) $(BASE_DIR) $(SHA) && cat $(TMP)/.defines)
CROSS_COMPILE:=$(shell $(__PYTHON) $(MAKE_PY) --cross-compile $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.cross-compile)
DEVICES:=$(shell $(__PYTHON) $(MAKE_PY) --devices $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.devices)
SERVER:=$(shell $(__PYTHON) $(MAKE_PY) --server-name $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.server-name)
MIDWARE_PATH=$(shell $(__PYTHON) $(MAKE_PY) --midware-path $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.midware-path)

KS_DEVICES=$(addprefix ks_,$(notdir $(filter-out %.cpp,$(DEVICES))))

__MIDWARE_PATH=$(BASE_DIR)/$(MIDWARE_PATH)

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

CFLAGS=-Wall -Werror -Wno-unknown-pragmas $(INC) $(DEFINES)

CFLAGS += $(ARCH_FLAGS)
CFLAGS += $(DEBUG_FLAGS)
CFLAGS += $(OPTIM_FLAGS)

CXXFLAGS=$(CFLAGS) -std=c++14 -pthread

# --------------------------------------------------------------
# Libraries
# --------------------------------------------------------------

LIBS = -lm # -lpthread -lssl -lcrypto

# http://bruno.defraine.net/techtips/makefile-auto-dependencies-with-gcc/
# DEP=$(CORE_SRC:.cpp=.d)
# -include $(DEP)

.PHONY: all debug requirements clean start_server stop_server test_python

all: $(EXECUTABLE)

debug:
	@echo TMP_CORE_OBJ = $(TMP_CORE_OBJ)
	@echo DEVICES = $(DEVICES)
	@echo KS_DEVICES = $(KS_DEVICES)

# ------------------------------------------------------------------------------------------------------------
# Build, start, stop
# ------------------------------------------------------------------------------------------------------------

$(TMP): requirements
	mkdir -p $(TMP)
	# mkdir -p $(TMP)/middleware
	$(__PYTHON) $(MAKE_PY) --generate $(CONFIG_PATH) $(BASE_DIR) $(TMP)

$(TMP)/%.o: */*/%.cpp
	$(CCXX) -c $(CXXFLAGS) -o $@ $<

$(TMP)/%.o: */%.cpp
	$(CCXX) -c $(CXXFLAGS) -o $@ $<

$(TMP)/%.o: $(TMP)/%.cpp
	$(CCXX) -c $(CXXFLAGS) -o $@ $<

# $(TMP)/%: $(CORE)/%
# 	cp $^ $@

requirements: $(MAKE_PY) $(CONFIG_PATH)
	$(__PYTHON) $(MAKE_PY) --requirements $(CONFIG_PATH) $(BASE_DIR)

$(EXECUTABLE): | $(TMP) $(CORE_HEADERS) $(TMP_CORE_OBJ) $(addprefix $(TMP)/,$(KS_DEVICES))
	$(CCXX) -o $@ $(wildcard $(TMP)/*.o) $(CXXFLAGS) $(LIBS)

# $(EXECUTABLE): $(CORE_HEADERS) $(TMP_CORE_OBJ) $(TMP)/main.cpp $(TMP)/Makefile $(MAKE_PY) $(CONFIG_PATH) | $(TMP)
# 	$(__PYTHON) $(MAKE_PY) --generate $(CONFIG_PATH) $(BASE_DIR) $(__MIDWARE_PATH)
# 	make -C $(TMP) CROSS_COMPILE=$(CROSS_COMPILE) DEFINES=$(DEFINES) SERVER=$(SERVER)             \
# 	               ARCH_FLAGS=$(ARCH_FLAGS) OPTIM_FLAGS=$(OPTIM_FLAGS) DEBUG_FLAGS=$(DEBUG_FLAGS) \
# 	               MIDWARE_PATH=$(__MIDWARE_PATH)

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
