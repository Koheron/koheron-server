# -------------------------------------------------------------------------------------
# Build and run koheron-server in local
# -------------------------------------------------------------------------------------

SERVER_URL = https://github.com/Koheron/koheron-server.git
SERVER_BRANCH = master
SERVER_DIR = $(TMP)/koheron-server
SERVER_BIN = $(SERVER_DIR)/tmp/kserverd
SERVER_VENV = $(SERVER_DIR)/koheron_server_venv

$(SERVER_DIR):
	git clone $(SERVER_URL) $(SERVER_DIR)
	cd $(SERVER_DIR) && git checkout $(SERVER_BRANCH)

$(SERVER_DIR)/requirements.txt: $(SERVER_DIR)

$(SERVER_VENV): $(SERVER_DIR)/requirements.txt
	virtualenv $(SERVER_VENV)
	$(SERVER_VENV)/bin/pip install -r $(SERVER_DIR)/requirements.txt

$(SERVER_BIN): $(SERVER_VENV)
	make -C $(SERVER_DIR) CONFIG=config/config_local.yaml PYTHON=koheron_server_venv/bin/python

start_server: $(SERVER_BIN)
	make -C $(SERVER_DIR) PYTHON=koheron_server_venv/bin/python start_server