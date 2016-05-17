# 'make' builds everything
# 'make clean' deletes everything except source files and Makefile

CONFIG=config_local.yaml

DOCKER=False
BUILD_LOCAL=False
USE_EIGEN = False

MIDWARE_PATH = middleware
REMOTE_DRIVERS = $(MIDWARE_PATH)/drivers
ZYNQ_SDK_PATH = tmp/zynq-sdk

current_dir := $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST))))

all: kserverd

$(REMOTE_DRIVERS):
ifeq ($(BUILD_LOCAL),True)
	git clone https://github.com/Koheron/zynq-sdk.git $(ZYNQ_SDK_PATH)
	cd $(ZYNQ_SDK_PATH) && git checkout master
	mkdir -p $(REMOTE_DRIVERS)/lib
	cp -r $(ZYNQ_SDK_PATH)/drivers/lib/. $(REMOTE_DRIVERS)/lib
	cp -r $(ZYNQ_SDK_PATH)/drivers/device_memory/. $(REMOTE_DRIVERS)
endif

libraries:
ifeq ($(USE_EIGEN),True)
	wget -P tmp bitbucket.org/eigen/eigen/get/3.2.6.tar.gz
	cd tmp && tar -zxvf 3.2.6.tar.gz
	mkdir -p $(MIDWARE_PATH)/libraries
	cp -r tmp/eigen-eigen-c58038c56923/Eigen $(MIDWARE_PATH)/libraries
endif

venv:
ifeq ($(DOCKER),False)
	virtualenv venv
	venv/bin/pip install -r requirements.txt
else
	pip install -r $(current_dir)/requirements.txt
endif

kserverd: venv libraries $(REMOTE_DRIVERS)
ifeq ($(DOCKER),False)
	venv/bin/python kmake.py kserver -c config/$(CONFIG) $(MIDWARE_PATH)
else
	python kmake.py kserver -c config/$(CONFIG) $(MIDWARE_PATH)
endif

clean:
	rm -rf tmp
	rm -rf venv
	rm -rf $(REMOTE_DRIVERS)

