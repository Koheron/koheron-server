CONFIG=tests/config_local.yaml
PYTHON=/usr/bin/python

SHA=`git rev-parse --short HEAD`
TAG=0.13.0

# Base directory for paths
BASE_DIR=.
CONFIG_PATH=$(BASE_DIR)/$(CONFIG)
TMP = $(BASE_DIR)/tmp

TEMPLATES = $(shell find scripts/templates -name '*.hpp' -o -name '*.h' -o -name '*.tpp')
CORE_HEADERS = $(shell find core -name '*.hpp' -o -name '*.h' -o -name '*.tpp')
CORE_SRC = $(shell find core -name '*.cpp' -o -name '*.c')

CORE_OBJ=$(subst .cpp,.o, $(addprefix $(TMP)/, $(notdir $(CORE_SRC))))

DEFINES:= -DDEBUG_KSERVER -DKOHERON_SERVER_VERSION=$(TAG).$(SHA)

DEVICES:= tests/tests.hpp tests/tests.cpp uses_context.hpp

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

VPATH=core:core/crypto:$(DEVICES_PATHS)
OBJ = $(CORE_OBJ) $(KS_DEVICES_OBJ) $(DEVICES_OBJ)
DEP=$(subst .o,.d,$(OBJ))
EXECUTABLE=$(TMP)/kserverd

# --------------------------------------------------------------
# Toolchain
# --------------------------------------------------------------
CROSS_COMPILE:=
ARCH_FLAGS:= -march=native

#CROSS_COMPILE:=/usr/bin/arm-linux-gnueabihf-
#ARCH_FLAGS:= -march=armv7-a -mtune=cortex-a9 -mfpu=vfpv3 -mfpu=neon -mfloat-abi=hard

# Use Link Time Optimization
CCXX=$(CROSS_COMPILE)g++ -flto

# --------------------------------------------------------------
# GCC compiling & linking flags
# --------------------------------------------------------------

INC=-I$(TMP) -I$(BASE_DIR) -I.
CFLAGS=-Wall -Werror $(INC) $(DEFINES) -MMD -MP $(ARCH_FLAGS) -O3
CXXFLAGS=$(CFLAGS) -std=c++14 -pthread

# --------------------------------------------------------------
# Libraries
# --------------------------------------------------------------

LIBS = -lm

.PHONY: all debug

all: exec

# ------------------------------------------------------------------------------------------------------------
# Build, start, stop
# ------------------------------------------------------------------------------------------------------------

.PHONY: exec operations_hpp

$(TMP)/devices_table.hpp $(TMP)/devices.hpp $(TMP)/operations.hpp $(KS_DEVICES_CPP): $(DEVICES_HPP) scripts/devgen.py $(TEMPLATES)
	python scripts/make.py --generate $(CONFIG_PATH) $(TMP)

$(TMP)/%.o: %.cpp
	$(CCXX) -c $(CXXFLAGS) -o $@ $<

$(TMP)/%.o: $(TMP)/%.cpp
	$(CCXX) -c $(CXXFLAGS) -o $@ $<

$(EXECUTABLE): $(OBJ)
	$(CCXX) -o $@ $(OBJ) $(CXXFLAGS) $(LIBS)

exec: $(TMP_DEVICE_TABLE_HPP) $(TMP_DEVICES_HPP) $(KS_DEVICES_CPP)
	$(MAKE) --jobs=$(shell nproc 2> /dev/null || echo 1) $(EXECUTABLE)

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
