# Websocket client tests
# (c) Koheron

# http://stackoverflow.com/questions/19971713/nodeunit-runtime-thrown-errors-in-test-function-are-silent/20038890#20038890
process.on('uncaughtException', (err) ->
  console.error err.stack
)

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
            get_cstr       : @device.getCmdRef( "GET_CSTR"       )
            get_tuple      : @device.getCmdRef( "GET_TUPLE"      )

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

    readString : (cb) ->
        @kclient.readString(Command(@id, @cmds.get_cstr), cb)

    readTuple : (cb) ->
        @kclient.readTuple(Command(@id, @cmds.get_tuple), cb)

# Unit tests

client = new websock_client.KClient('127.0.0.1', 1)

exports.readUint = (assert) ->
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readUint( (num) =>
                assert.equals(num, 42)
                client.exit()
                assert.done()
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
                client.exit()
                assert.done()
            )
        )
    )

exports.rcvStdArray = (assert) ->
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.rcvStdArray( (array) =>
                is_ok = true
                for i in [0..array.length-1]
                    if array[i] != i*i
                        is_ok = false
                        break

                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.readString = (assert) ->
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readString( (str) =>
                assert.equals(str, 'Hello !')
                client.exit()
                assert.done()
            )
        )
    )

exports.readTuple = (assert) ->
    assert.expect(7)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readTuple( (tuple) =>
                assert.equals(tuple[0].type, 'int')
                assert.equals(tuple[0].value, 2)
                assert.equals(tuple[1].type, 'float')
                assert.equals(tuple[1].value, 3.14159)
                assert.equals(tuple[2].type, 'double')
                assert.equals(tuple[2].value, 2345.6)
                client.exit()
                assert.done()
            )
        )
    )