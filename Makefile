# 'make' builds everything
# 'make clean' deletes everything except source files and Makefile

CONFIG=config_local.yaml

DOCKER=False
BUILD_LOCAL=False
USE_EIGEN = False

TMP = tmp
CORE = core
MIDDLEWARE = middleware

ZYNQ_SDK = $(TMP)/zynq-sdk

ifeq ($(DOCKER),False)
	PIP=venv/bin/pip
	PYTHON=venv/bin/python
else
	PIP=pip
	PYTHON=python
endif

current_dir := $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST))))

all: kserverd

$(ZYNQ_SDK):
	git clone https://github.com/Koheron/zynq-sdk.git $(ZYNQ_SDK)
	cd $(ZYNQ_SDK) && git checkout master

$(TMP): $(CORE) $(MIDDLEWARE) $(ZYNQ_SDK)
	mkdir -p $(TMP)
	mkdir -p $(TMP)/devices
	cp -r $(CORE) $(TMP)/core
	rm -f $(TMP)/core/Makefile
	rm -f $(TMP)/core/main.cpp
	cp -r $(MIDDLEWARE) $(TMP)/middleware
	cp $(CORE)/main.cpp $(TMP)/main.cpp

ifeq ($(BUILD_LOCAL),True)
	mkdir -p $(MIDDLEWARE)/drivers/lib
	cp -r $(ZYNQ_SDK)/drivers/lib/. $(TMP)/middleware/drivers/lib
	cp -r $(ZYNQ_SDK)/drivers/device_memory/. $(TMP)/middleware/drivers
endif

libraries:
ifeq ($(USE_EIGEN),True)
	wget -P tmp bitbucket.org/eigen/eigen/get/3.2.6.tar.gz
	cd tmp && tar -zxvf 3.2.6.tar.gz
	mkdir -p $(MIDDLEWARE)/libraries
	cp -r tmp/eigen-eigen-c58038c56923/Eigen $(MIDDLEWARE)/libraries
endif

venv:
ifeq ($(DOCKER),False)
	virtualenv venv
	$(PIP) install -r requirements.txt
else
	$(PIP) install -r $(current_dir)/requirements.txt
endif

kserverd: venv libraries $(TMP)
	$(PYTHON) make.py config/$(CONFIG) $(MIDDLEWARE)

clean:
	rm -rf $(TMP)
	rm -rf venv

