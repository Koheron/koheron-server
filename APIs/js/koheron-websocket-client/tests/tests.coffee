websock_client = require('../lib/koheron-websocket-client.js')

client = new websock_client.KClient('127.0.0.1', 4)

client.init( ->
	console.log "Connection initialized"
)