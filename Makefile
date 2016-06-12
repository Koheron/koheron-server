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

CORE_SRC=$(shell find $(CORE) -name '*.cpp' -o -name '*.c' -o -name '*.hpp' -o -name '*.h' -o -name '*.tpp')
TMP_CORE_SRC=$(addprefix $(TMP)/, $(CORE_SRC))

ARCH_FLAGS:=$(shell $(__PYTHON) $(MAKE_PY) --arch-flags $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.arch-flags)
OPTIM_FLAGS:=$(shell $(__PYTHON) $(MAKE_PY) --optim-flags $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.optim-flags)
DEBUG_FLAGS:=$(shell $(__PYTHON) $(MAKE_PY) --debug-flags $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.debug-flags)
DEFINES:=$(shell $(__PYTHON) $(MAKE_PY) --defines $(CONFIG_PATH) $(BASE_DIR) $(SHA) && cat $(TMP)/.defines)
CROSS_COMPILE:=$(shell $(__PYTHON) $(MAKE_PY) --cross-compile $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.cross-compile)
DEVICES:=$(shell $(__PYTHON) $(MAKE_PY) --devices $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.devices)
SERVER:=$(shell $(__PYTHON) $(MAKE_PY) --server-name $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.server-name)
MIDWARE_PATH=$(shell $(__PYTHON) $(MAKE_PY) --midware-path $(CONFIG_PATH) $(BASE_DIR) && cat $(TMP)/.midware-path)

__MIDWARE_PATH=$(BASE_DIR)/$(MIDWARE_PATH)

EXECUTABLE=$(TMP)/$(SERVER)

all: $(EXECUTABLE)

$(TMP): requirements
	mkdir -p $(TMP)
	mkdir -p $(TMP)/middleware

$(TMP)/$(CORE)/%: $(CORE)/%
	mkdir -p -- `dirname -- $@`
	cp $^ $@

$(TMP)/%: $(CORE)/%
	cp $^ $@

requirements: $(MAKE_PY) $(CONFIG_PATH) $(DEVICES)
	$(__PYTHON) $(MAKE_PY) --requirements $(CONFIG_PATH) $(BASE_DIR)

$(EXECUTABLE): $(TMP_CORE_SRC) $(TMP)/main.cpp $(TMP)/Makefile $(MAKE_PY) $(CONFIG_PATH) | $(TMP)
	$(__PYTHON) $(MAKE_PY) --generate $(CONFIG_PATH) $(BASE_DIR) $(__MIDWARE_PATH)
	make -C $(TMP) CROSS_COMPILE=$(CROSS_COMPILE) DEFINES=$(DEFINES) SERVER=$(SERVER)             \
	               ARCH_FLAGS=$(ARCH_FLAGS) OPTIM_FLAGS=$(OPTIM_FLAGS) DEBUG_FLAGS=$(DEBUG_FLAGS) \
	               MIDWARE_PATH=$(__MIDWARE_PATH)

cli:
	make -C apis/cli CROSS_COMPILE=$(CROSS_COMPILE) DEFINES=$(DEFINES) ARCH_FLAGS=$(ARCH_FLAGS)

clean:
	rm -rf $(TMP)
	make -C apis/cli clean
