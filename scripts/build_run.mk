# -------------------------------------------------------------------------------------
# Build and run koheron-server in local
#
# In your Makefile:
# KOHERON_SERVER_DEST=$(TMP)
# DUMMY:=$(shell curl https://raw.githubusercontent.com/Koheron/koheron-server/master/scripts/build_run.mk > build_run.mk)
# include build_run.mk
# -------------------------------------------------------------------------------------

KOHERON_SERVER_URL = https://github.com/Koheron/koheron-server.git

ifeq ($(KOHERON_SERVER_BRANCH),)
KOHERON_SERVER_BRANCH = master
endif

KOHERON_SERVER_DIR = $(KOHERON_SERVER_DEST)/koheron-server
KOHERON_SERVER_BIN = $(KOHERON_SERVER_DIR)/tmp/kserverd
KOHERON_SERVER_VENV = $(KOHERON_SERVER_DIR)/koheron_server_venv

.PHONY: start_koheron_server stop_koheron_server operations_hpp

$(KOHERON_SERVER_DIR):
	git clone $(KOHERON_SERVER_URL) $(KOHERON_SERVER_DIR)
	cd $(KOHERON_SERVER_DIR) && git checkout $(KOHERON_SERVER_BRANCH)

$(KOHERON_SERVER_DIR)/requirements.txt: $(KOHERON_SERVER_DIR)

$(KOHERON_SERVER_VENV): $(KOHERON_SERVER_DIR)/requirements.txt
	test -d $(KOHERON_SERVER_VENV) || (virtualenv $(KOHERON_SERVER_VENV) && $(KOHERON_SERVER_VENV)/bin/pip install -r $(KOHERON_SERVER_DIR)/requirements.txt)

$(KOHERON_SERVER_BIN): $(KOHERON_SERVER_VENV)
	make -C $(KOHERON_SERVER_DIR) CONFIG=config/config_local.yaml PYTHON=koheron_server_venv/bin/python

operations_hpp:
	make -C $(KOHERON_SERVER_DIR) PYTHON=koheron_server_venv/bin/python operations_hpp

start_koheron_server: $(KOHERON_SERVER_BIN)
	make -C $(KOHERON_SERVER_DIR) PYTHON=koheron_server_venv/bin/python start_server

stop_koheron_server:
	make -C $(KOHERON_SERVER_DIR) PYTHON=koheron_server_venv/bin/python stop_server