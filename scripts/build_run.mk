# -------------------------------------------------------------------------------------
# Build and run koheron-server in local
#
#
# Usage: in your Makefile
# KOHERON_SERVER_DEST = tmp
# include build_run.mk
# -------------------------------------------------------------------------------------

KOHERON_SERVER_URL = https://github.com/Koheron/koheron-server.git
KOHERON_SERVER_BRANCH = master
KOHERON_SERVER_DIR = $(KOHERON_SERVER_DEST)/koheron-server
KOHERON_SERVER_BIN = $(KOHERON_SERVER_DIR)/tmp/kserverd
KOHERON_SERVER_VENV = $(KOHERON_SERVER_DIR)/koheron_server_venv

$(KOHERON_SERVER_DIR):
	git clone $(KOHERON_SERVER_URL) $(KOHERON_SERVER_DIR)
	cd $(KOHERON_SERVER_DIR) && git checkout $(KOHERON_SERVER_BRANCH)

$(KOHERON_SERVER_DIR)/requirements.txt: $(KOHERON_SERVER_DIR)

$(KOHERON_SERVER_VENV): $(KOHERON_SERVER_DIR)/requirements.txt
	virtualenv $(KOHERON_SERVER_VENV)
	$(KOHERON_SERVER_VENV)/bin/pip install -r $(KOHERON_SERVER_DIR)/requirements.txt

$(KOHERON_SERVER_BIN): $(KOHERON_SERVER_VENV)
	make -C $(KOHERON_SERVER_DIR) CONFIG=config/config_local.yaml PYTHON=koheron_server_venv/bin/python

start_server: $(KOHERON_SERVER_BIN)
	make -C $(KOHERON_SERVER_DIR) PYTHON=koheron_server_venv/bin/python start_server