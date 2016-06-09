
CONFIG=config/config_local.yaml

PYTHON=/usr/bin/python

USE_EIGEN = False

TMP = tmp
CORE = core

MAKE_PY = make.py

ARCH_FLAGS:=$(shell $(PYTHON) $(MAKE_PY) --arch-flags $(CONFIG) && cat $(TMP)/.arch-flags)
DEFINES:=$(shell $(PYTHON) $(MAKE_PY) --defines $(CONFIG) && cat $(TMP)/.defines)
CROSS_COMPILE:=$(shell $(PYTHON) $(MAKE_PY) --cross-compile $(CONFIG) && cat $(TMP)/.cross-compile)
HOST:=$(shell $(PYTHON) $(MAKE_PY) --host $(CONFIG) && cat $(TMP)/.host)
DEVICES:=$(shell $(PYTHON) $(MAKE_PY) --devices $(CONFIG) && cat $(TMP)/.devices)

all: kserverd

$(TMP): requirements $(CORE) $(DEVICES)
	mkdir -p $(TMP)
	mkdir -p $(TMP)/devices
	mkdir -p $(TMP)/middleware
	cp -r $(CORE) $(TMP)/core
	rm -f $(TMP)/core/Makefile
	rm -f $(TMP)/core/main.cpp
	cp $(CORE)/main.cpp $(TMP)/main.cpp
	cp -r $(DEVICES) $(TMP)/middleware

libraries:
ifeq ($(USE_EIGEN),True)
	wget -P tmp bitbucket.org/eigen/eigen/get/3.2.6.tar.gz
	cd tmp && tar -zxvf 3.2.6.tar.gz
	mkdir -p $(MIDDLEWARE)/libraries
	cp -r tmp/eigen-eigen-c58038c56923/Eigen $(MIDDLEWARE)/libraries
endif

requirements: $(MAKE_PY) $(CONFIG)
	$(PYTHON) $(MAKE_PY) --requirements $(CONFIG)

kserverd: $(TMP) $(MAKE_PY) $(CONFIG) libraries
	$(PYTHON) $(MAKE_PY) --generate $(CONFIG)
	make -C $(TMP) TARGET_HOST=$(HOST) CROSS_COMPILE=$(CROSS_COMPILE) DEFINES=$(DEFINES) ARCH_FLAGS=$(ARCH_FLAGS)

clean:
	rm -rf $(TMP)
