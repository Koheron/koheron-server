CONFIG=config/config_local.yaml
PYTHON=/usr/bin/python

# Base directory for paths
BASE_DIR=.
CONFIG_PATH=$(BASE_DIR)/$(CONFIG)
__PYTHON = $(shell bash scripts/get_python.sh $(PYTHON) $(BASE_DIR))

TMP = tmp
CORE = core
MAKE_PY = make.py

ARCH_FLAGS:=$(shell $(__PYTHON) $(MAKE_PY) --arch-flags $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.arch-flags)
OPTIM_FLAGS:=$(shell $(__PYTHON) $(MAKE_PY) --optim-flags $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.optim-flags)
DEBUG_FLAGS:=$(shell $(__PYTHON) $(MAKE_PY) --debug-flags $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.debug-flags)
DEFINES:=$(shell $(__PYTHON) $(MAKE_PY) --defines $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.defines)
CROSS_COMPILE:=$(shell $(__PYTHON) $(MAKE_PY) --cross-compile $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.cross-compile)
DEVICES:=$(shell $(__PYTHON) $(MAKE_PY) --devices $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.devices)
SERVER:=$(shell $(__PYTHON) $(MAKE_PY) --server-name $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.server-name)
MIDWARE_PATH=$(shell $(__PYTHON) $(MAKE_PY) --midware-path $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.midware-path)

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

requirements: $(MAKE_PY) $(CONFIG_PATH)
	$(__PYTHON) $(MAKE_PY) --requirements $(CONFIG_PATH) $(BASE_DIR)

$(EXECUTABLE): $(TMP) $(MAKE_PY) $(CONFIG_PATH)
	$(__PYTHON) $(MAKE_PY) --generate $(CONFIG_PATH) $(BASE_DIR) $(MIDWARE_PATH)
	make -C $(TMP) CROSS_COMPILE=$(CROSS_COMPILE) DEFINES=$(DEFINES) SERVER=$(SERVER)             \
	               ARCH_FLAGS=$(ARCH_FLAGS) OPTIM_FLAGS=$(OPTIM_FLAGS) DEBUG_FLAGS=$(DEBUG_FLAGS) \
	               MIDWARE_PATH=$(MIDWARE_PATH)

cli:
	make -C APIs/cli CROSS_COMPILE=$(CROSS_COMPILE) DEFINES=$(DEFINES) ARCH_FLAGS=$(ARCH_FLAGS)

clean:
	rm -rf $(TMP)
	make -C APIs/cli clean