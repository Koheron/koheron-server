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

include tests/build.mk


DEVICES_HPP=$(filter-out %.cpp,$(DEVICES))
DEVICES_CPP=$(filter-out %.hpp,$(DEVICES))
DEVICES_OBJ=$(addprefix $(TMP)/, $(subst .cpp,.o,$(notdir $(filter-out %.hpp,$(DEVICES)))))

# Generated source files
KS_DEVICES_HPP=$(addprefix ks_,$(notdir $(DEVICES_HPP)))
KS_DEVICES_CPP=$(addprefix $(TMP)/,$(subst .hpp,.cpp,$(KS_DEVICES_HPP)))
KS_DEVICES_OBJ=$(addprefix $(TMP)/,$(subst .hpp,.o,$(KS_DEVICES_HPP))) $(TMP)/context.o

VPATH=core:core/crypto:$(_VPATH)
OBJ = $(CORE_OBJ) $(KS_DEVICES_OBJ) $(DEVICES_OBJ)
DEP=$(subst .o,.d,$(OBJ))
EXECUTABLE=$(TMP)/kserverd

# --------------------------------------------------------------
# GCC compiling & linking flags
# --------------------------------------------------------------

INC=-I$(TMP) -I$(BASE_DIR) -I.
CFLAGS=-Wall -Werror $(INC) -DKOHERON_SERVER_VERSION=$(TAG).$(SHA) -MMD -MP $(ARCH_FLAGS) -O3
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
# Clean
# ------------------------------------------------------------------------------------------------------------

.PHONY: clean clean_venv

clean_venv:
	rm -rf $(TESTS_VENV)

clean:
	rm -rf $(TMP)
