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
            set_mean       : @device.getCmdRef( "SET_MEAN"       )
            set_std_dev    : @device.getCmdRef( "SET_STD_DEV"    )
            send_std_array : @device.getCmdRef( "SEND_STD_ARRAY" )
            set_buffer     : @device.getCmdRef( "SET_BUFFER"     )
            read_uint      : @device.getCmdRef( "READ_UINT"      )
            read_int       : @device.getCmdRef( "READ_INT"       )

    setMean : (mean) ->
        @kclient.send(Command(@id, @cmds.set_mean, 'f', mean))

    setStdDev : (std) ->
        @kclient.send(Command(@id, @cmds.set_std_dev, 'f', std))

    rcvStdArray : (cb) ->
        @kclient.readFloat32Array(Command(@id, @cmds.send_std_array), cb)

    sendBuffer : (len) ->
        buffer = new Uint32Array(len)
        (buffer[i] = i*i for i in [0..len])
        @kclient.sendArray(Command(@id, @cmds.set_buffer, 'u', buffer.length), buffer)

    readUint : (cb) ->
        @kclient.readUint32(Command(@id, @cmds.read_uint), cb)

    readInt : (cb) ->
        @kclient.readInt32(Command(@id, @cmds.read_int), cb)

# Tests

# socketpool_size = 4
# client = new websock_client.KClient('127.0.0.1', socketpool_size)

# client.init( ->
#     console.log "Connection initialized"

#     tests = new Tests(client)
#     tests.setMean(12.5)
#     tests.setStdDev(2.3)
#     tests.rcvStdArray( (array) -> console.log array )
#     tests.sendBuffer(10)
#     tests.readUint( (num) -> console.log num)

#     # client.exit()
# )

client = new websock_client.KClient('127.0.0.1', 2)

exports.readUint = (assert) ->
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readUint( (num) =>
                assert.equals(num, 42)
                assert.done()
                client.exit()
            )
        )
    )

exports.readInt = (assert) ->
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readInt( (num) =>
                assert.equals(num, -42)
                assert.done()
                client.exit()
            )
        )
    )

exports.setMean = (assert) ->
    assert.expect(1)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.setMean(12.5)
            assert.done()
            client.exit()
        )
    )