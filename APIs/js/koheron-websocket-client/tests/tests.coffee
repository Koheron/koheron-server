# Websocket client tests
# (c) Koheron

socketpool_size = 4
websock_client = require('../lib/koheron-websocket-client.js')

client = new websock_client.KClient('127.0.0.1', socketpool_size)

client.init( ->
    console.log "Connection initialized"
    client.exit()
)

