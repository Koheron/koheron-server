
CONFIG=config/config_local.yaml

BUILD_LOCAL=False
USE_EIGEN = False

TMP = tmp
CORE = core

MAKE_PY = make.py

ZYNQ_SDK = $(TMP)/zynq-sdk

ARCH_FLAGS:=$(shell python $(MAKE_PY) --arch-flags $(CONFIG) && cat $(TMP)/.arch-flags)
DEFINES:=$(shell python $(MAKE_PY) --defines $(CONFIG) && cat $(TMP)/.defines)
CROSS_COMPILE:=$(shell python $(MAKE_PY) --cross-compile $(CONFIG) && cat $(TMP)/.cross-compile)
HOST:=$(shell python $(MAKE_PY) --host $(CONFIG) && cat $(TMP)/.host)
DEVICES:=$(shell python $(MAKE_PY) --devices $(CONFIG) && cat $(TMP)/.devices)

current_dir := $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST))))

all: kserverd

$(ZYNQ_SDK):
ifeq ($(BUILD_LOCAL),True)
	git clone https://github.com/Koheron/zynq-sdk.git $(ZYNQ_SDK)
	cd $(ZYNQ_SDK) && git checkout master
	mkdir -p $(TMP)/middleware/drivers/lib
	cp -r $(ZYNQ_SDK)/drivers/lib/. $(TMP)/middleware/drivers/lib
endif

$(TMP): $(ZYNQ_SDK) $(CORE) $(DEVICES)
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

kserverd: libraries $(TMP) $(MAKE_PY)
	python $(MAKE_PY) --generate $(CONFIG)
	make -C $(TMP) TARGET_HOST=$(HOST) CROSS_COMPILE=$(CROSS_COMPILE) DEFINES=$(DEFINES) ARCH_FLAGS=$(ARCH_FLAGS)

clean:
	rm -rf $(TMP)
