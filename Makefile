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


TEMPLATES = $(shell find scripts/templates -name '*.hpp' -o -name '*.h' -o -name '*.tpp')
CORE_HEADERS=$(shell find core -name '*.hpp' -o -name '*.h' -o -name '*.tpp')
CORE_SRC=$(shell find core -name '*.cpp' -o -name '*.c')
CORE_OBJ=$(subst .cpp,.o, $(addprefix $(TMP)/, $(notdir $(CORE_SRC))))

ARCH_FLAGS:= -march=armv7-a -mtune=cortex-a9 -mfpu=vfpv3 -mfpu=neon -mfloat-abi=hard
OPTIM_FLAGS:= -O3

DEFINES:= -DDEBUG_KSERVER -DKOHERON_SERVER_VERSION=$(KOHERON_SERVER_VERSION)
CROSS_COMPILE:=/usr/bin/arm-linux-gnueabihf-
DEVICES:= tests/tests.hpp tests/exception_tests.hpp

DEVICES_HPP=$(filter-out %.cpp,$(DEVICES))
DEVICES_CPP=$(filter-out %.hpp,$(DEVICES))
DEVICES_OBJ=$(addprefix $(TMP)/, $(subst .cpp,.o,$(notdir $(filter-out %.hpp,$(DEVICES)))))

_DEVICES_PATHS=$(sort $(dir $(DEVICES)))

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
OBJ = $(CORE_OBJ) $(KS_DEVICES_OBJ) $(DEVICES_OBJ)
DEP=$(subst .o,.d,$(OBJ))
EXECUTABLE=$(TMP)/kserverd

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
CFLAGS += $(ARCH_FLAGS) $(OPTIM_FLAGS)
CXXFLAGS=$(CFLAGS) -std=c++14 -pthread

# --------------------------------------------------------------
# Libraries
# --------------------------------------------------------------

LIBS = -lm # -lpthread -lssl -lcrypto

.PHONY: all debug

all: exec

# ------------------------------------------------------------------------------------------------------------
# Build, start, stop
# ------------------------------------------------------------------------------------------------------------

.PHONY: exec start_server stop_server operations_hpp

# http://bruno.defraine.net/techtips/makefile-auto-dependencies-with-gcc/
# http://scottmcpeak.com/autodepend/autodepend.html
-include $(DEP)

$(TMP_DEVICE_TABLE_HPP) $(TMP_DEVICES_HPP) $(TMP_OPERATIONS_HPP) $(KS_DEVICES_CPP): $(DEVICES_HPP) scripts/devgen.py $(TEMPLATES)
	$(__PYTHON) scripts/make.py --generate $(CONFIG_PATH) $(BASE_DIR) $(TMP)

$(TMP)/%.o: %.cpp
	$(CCXX) -c $(CXXFLAGS) -o $@ $<

$(TMP)/%.o: $(TMP)/%.cpp
	$(CCXX) -c $(CXXFLAGS) -o $@ $<

$(EXECUTABLE): $(OBJ)
	$(CCXX) -o $@ $(OBJ) $(CXXFLAGS) $(LIBS)

exec: $(TMP_DEVICE_TABLE_HPP) $(TMP_DEVICES_HPP) $(KS_DEVICES_CPP)
	$(MAKE) --jobs=$(shell nproc 2> /dev/null || echo 1) $(EXECUTABLE)

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
KOHERON_PYTHON_BRANCH = v0.13.0
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
