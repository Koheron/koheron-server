CONFIG=config/config_local.yaml
PYTHON=/usr/bin/python

SHA=`git rev-parse --short HEAD`
TAG=0.13.0
KOHERON_SERVER_VERSION=$(TAG).$(SHA)

# Base directory for paths
BASE_DIR=.
CONFIG_PATH=$(BASE_DIR)/$(CONFIG)
TMP = $(BASE_DIR)/tmp
__PYTHON = $(shell bash scripts/get_python.sh $(PYTHON) $(BASE_DIR))

CORE = core
MAKE_PY = scripts/make.py
DEVGEN_PY = scripts/devgen.py

FULL_CFG = $(TMP)/full_config.json
$(shell $(__PYTHON) $(MAKE_PY) --config $(CONFIG_PATH) $(BASE_DIR) $(TMP))

TEMPLATES = $(shell find scripts/templates -name '*.hpp' -o -name '*.h' -o -name '*.tpp')
CORE_HEADERS=$(shell find $(CORE) -name '*.hpp' -o -name '*.h' -o -name '*.tpp')
CORE_SRC=$(shell find $(CORE) -name '*.cpp' -o -name '*.c')
CORE_OBJ=$(subst .cpp,.o, $(addprefix $(TMP)/, $(notdir $(CORE_SRC))))

ARCH_FLAGS:=$(shell cat $(FULL_CFG) | $(__PYTHON) -c "import sys, json; cfg = json.load(sys.stdin); print '-' + ' -'.join(cfg['arch_flags'])")
OPTIM_FLAGS:=$(shell cat $(FULL_CFG) | $(__PYTHON) -c "import sys, json; cfg = json.load(sys.stdin); print '-' + ' -'.join(cfg['optimization_flags'])")
DEBUG_FLAGS:=$(shell $(__PYTHON) $(MAKE_PY) --debug-flags $(CONFIG_PATH) $(BASE_DIR) $(TMP) && cat $(TMP)/.debug-flags)
DEFINES:=$(shell cat $(FULL_CFG) | $(__PYTHON) -c "import sys, json; cfg = json.load(sys.stdin); print '-D' + ' -D'.join(cfg['defines']) + ' -DKOHERON_SERVER_VERSION=' + '$(KOHERON_SERVER_VERSION)'")
CROSS_COMPILE:=$(shell cat $(FULL_CFG) | $(__PYTHON) -c "import sys, json; print json.load(sys.stdin)['cross-compile']")
DEVICES:=$(shell $(__PYTHON) $(MAKE_PY) --devices $(CONFIG_PATH) $(BASE_DIR) $(TMP) && cat $(TMP)/.devices)
DEPENDENCIES:=$(shell $(__PYTHON) $(MAKE_PY) --dependencies $(CONFIG_PATH) $(BASE_DIR) $(TMP) && cat $(TMP)/.dependencies)
SERVER:=$(shell cat $(FULL_CFG) | $(__PYTHON) -c "import sys, json; print json.load(sys.stdin)['server-name']")

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
KS_DEVICES_OBJ=$(addprefix $(TMP)/,$(subst .hpp,.o,$(KS_DEVICES_HPP))) $(TMP)/context.o
TMP_DEVICE_TABLE_HPP=$(TMP)/devices_table.hpp
TMP_DEVICES_HPP=$(TMP)/devices.hpp
TMP_OPERATIONS_HPP=$(TMP)/operations.hpp

VPATH=core:core/crypto:$(DEVICES_PATHS)
OBJ = $(CORE_OBJ) $(KS_DEVICES_OBJ) $(DEVICES_OBJ) $(DEPENDENCIES_OBJ)
DEP=$(subst .o,.d,$(OBJ))
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

.PHONY: exec start_server stop_server operations_hpp

# http://bruno.defraine.net/techtips/makefile-auto-dependencies-with-gcc/
# http://scottmcpeak.com/autodepend/autodepend.html
-include $(DEP)

$(TMP_DEVICE_TABLE_HPP) $(TMP_DEVICES_HPP) $(TMP_OPERATIONS_HPP) $(KS_DEVICES_CPP): $(DEVICES_HPP) $(DEVGEN_PY) $(TEMPLATES)
	$(__PYTHON) $(MAKE_PY) --generate $(CONFIG_PATH) $(BASE_DIR) $(TMP)

$(TMP)/%.o: %.cpp
	$(CCXX) -c $(CXXFLAGS) -o $@ $<

$(TMP)/%.o: $(TMP)/%.cpp
	$(CCXX) -c $(CXXFLAGS) -o $@ $<

$(EXECUTABLE): $(OBJ)
	$(CCXX) -o $@ $(OBJ) $(CXXFLAGS) $(LIBS)

exec: $(TMP_DEVICE_TABLE_HPP) $(TMP_DEVICES_HPP) $(KS_DEVICES_CPP)
	$(MAKE) --jobs=$(CPUS) $(EXECUTABLE)

start_server: exec stop_server
	nohup $(EXECUTABLE) -c config/kserver_local.conf > /dev/null 2> server.log &

stop_server:
	-pkill -SIGINT $(SERVER) # We ignore the error raised if the server is already stopped

operations_hpp: $(TMP_OPERATIONS_HPP)

# ------------------------------------------------------------------------------------------------------------
# Test python API
# ------------------------------------------------------------------------------------------------------------

.PHONY: test_python

KOHERON_PYTHON_URL = https://github.com/Koheron/koheron-python.git
KOHERON_PYTHON_BRANCH = v0.12.0
KOHERON_PYTHON_DIR = $(TMP)/koheron-python

$(KOHERON_PYTHON_DIR):
	git clone $(KOHERON_PYTHON_URL) $(KOHERON_PYTHON_DIR)
	cd $(KOHERON_PYTHON_DIR) && git checkout $(KOHERON_PYTHON_BRANCH)

test_python: $(KOHERON_PYTHON_DIR) start_server
	make -C $(KOHERON_PYTHON_DIR) test

# ------------------------------------------------------------------------------------------------------------
# Clean
# ------------------------------------------------------------------------------------------------------------

.PHONY: clean clean_venv

clean_venv:
	rm -rf $(TESTS_VENV)

clean:
	rm -rf $(TMP)
