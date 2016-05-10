# Websocket client tests
# (c) Koheron

websock_client = require('../lib/koheron-websocket-client.js')
Command = websock_client.Command

class Tests
    "use strict"

    constructor : (@kclient) ->
        @device = @kclient.getDevice("TESTS")
        @id = @device.id
    
        @cmds =
            set_mean : @device.getCmdRef( "SET_MEAN" )

    setMean : (mean) ->
        @kclient.send(Command(@id, @cmds.set_mean, 'f', mean))

# Tests

socketpool_size = 4
client = new websock_client.KClient('127.0.0.1', socketpool_size)

client.init( ->
    console.log "Connection initialized"

    tests = new Tests(client)
    tests.setMean(12.5)

    client.exit()
)

