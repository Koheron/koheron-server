
# Base directory for paths.
# Root of the repository by default.
BASE_DIR=.
CONFIG=config/config_local.yaml

CONFIG_PATH=$(BASE_DIR)/$(CONFIG)

PYTHON=/usr/bin/python

TMP = tmp
CORE = core
MAKE_PY = make.py

ARCH_FLAGS:=$(shell $(PYTHON) $(MAKE_PY) --arch-flags $(CONFIG_PATH) && cat $(TMP)/.arch-flags)
OPTIM_FLAGS:=$(shell $(PYTHON) $(MAKE_PY) --optim-flags $(CONFIG_PATH) && cat $(TMP)/.optim-flags)
DEBUG_FLAGS:=$(shell $(PYTHON) $(MAKE_PY) --debug-flags $(CONFIG_PATH) && cat $(TMP)/.debug-flags)
DEFINES:=$(shell $(PYTHON) $(MAKE_PY) --defines $(CONFIG_PATH) && cat $(TMP)/.defines)
CROSS_COMPILE:=$(shell $(PYTHON) $(MAKE_PY) --cross-compile $(CONFIG_PATH) && cat $(TMP)/.cross-compile)
DEVICES:=$(shell $(PYTHON) $(MAKE_PY) --devices $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.devices)
SERVER:=$(shell $(PYTHON) $(MAKE_PY) --server-name $(CONFIG_PATH) && cat $(TMP)/.server-name)

EXECUTABLE=$(TMP)/$(SERVER)

all: $(EXECUTABLE)

$(TMP): requirements $(CORE) $(DEVICES)
	mkdir -p $(TMP)
	mkdir -p $(TMP)/devices
	mkdir -p $(TMP)/middleware
	cp -r $(CORE) $(TMP)/core
	rm -f $(TMP)/core/Makefile
	rm -f $(TMP)/core/main.cpp
	cp $(CORE)/main.cpp $(TMP)/main.cpp
	cp -r $(DEVICES) $(TMP)/middleware

requirements: $(MAKE_PY) $(CONFIG_PATH)
	$(PYTHON) $(MAKE_PY) --requirements $(CONFIG_PATH) $(BASE_DIR)

$(EXECUTABLE): $(TMP) $(MAKE_PY) $(CONFIG_PATH)
	$(PYTHON) $(MAKE_PY) --generate $(CONFIG_PATH)
	make -C $(TMP) CROSS_COMPILE=$(CROSS_COMPILE) DEFINES=$(DEFINES) SERVER=$(SERVER) \
	               ARCH_FLAGS=$(ARCH_FLAGS) OPTIM_FLAGS=$(OPTIM_FLAGS) DEBUG_FLAGS=$(DEBUG_FLAGS)

clean:
	rm -rf $(TMP)