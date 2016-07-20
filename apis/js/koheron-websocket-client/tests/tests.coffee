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
            rcv_many_params : @device.getCmdRef( "RCV_MANY_PARAMS" )
            set_float       : @device.getCmdRef( "SET_FLOAT"       )
            send_std_vector : @device.getCmdRef( "SEND_STD_VECTOR" )
            send_std_array  : @device.getCmdRef( "SEND_STD_ARRAY"  )
            send_c_array1   : @device.getCmdRef( "SEND_C_ARRAY1"   )
            send_c_array2   : @device.getCmdRef( "SEND_C_ARRAY2"   )
            set_buffer      : @device.getCmdRef( "SET_BUFFER"      )
            read_uint       : @device.getCmdRef( "READ_UINT"       )
            read_int        : @device.getCmdRef( "READ_INT"        )
            read_float      : @device.getCmdRef( "READ_FLOAT"      )
            read_double     : @device.getCmdRef( "READ_DOUBLE"     )
            get_cstr        : @device.getCmdRef( "GET_CSTR"        )
            get_tuple       : @device.getCmdRef( "GET_TUPLE"       )
            get_tuple3      : @device.getCmdRef( "GET_TUPLE3"      )

    sendManyParams : (u1, u2, f, b, cb) ->
        @kclient.readBool(Command(@id, @cmds.rcv_many_params, 'IIf?', u1, u2, f, b), cb)

    setFloat : (f, cb) ->
        @kclient.readBool(Command(@id, @cmds.set_float, 'f', f), cb)

    rcvStdVector : (cb) ->
        @kclient.readFloat32Array(Command(@id, @cmds.send_std_vector), cb)

    rcvStdArray : (cb) ->
        @kclient.readFloat32Array(Command(@id, @cmds.send_std_array), cb)

    rcvCArray1 : (len, cb) ->
        @kclient.readFloat32Array(Command(@id, @cmds.send_c_array1, 'I', len), cb)

    rcvCArray2 : (cb) ->
        @kclient.readFloat32Array(Command(@id, @cmds.send_c_array2), cb)

    sendBuffer : (len, cb) ->
        buffer = new Uint32Array(len)
        (buffer[i] = i*i for i in [0..len])
        @kclient.sendArray(Command(@id, @cmds.set_buffer, 'I', buffer.length), buffer, (ok) -> cb(ok==1))

    readUint : (cb) ->
        @kclient.readUint32(Command(@id, @cmds.read_uint), cb)

    readInt : (cb) ->
        @kclient.readInt32(Command(@id, @cmds.read_int), cb)

    readFloat : (cb) ->
        @kclient.readFloat32(Command(@id, @cmds.read_float), cb)

    readDouble : (cb) ->
        @kclient.readFloat64(Command(@id, @cmds.read_double), cb)

    readString : (cb) ->
        @kclient.readString(Command(@id, @cmds.get_cstr), cb)

    readTuple : (cb) ->
        @kclient.readTuple(Command(@id, @cmds.get_tuple), 'Ifd?', cb)

    readTuple3 : (cb) ->
        @kclient.readTuple(Command(@id, @cmds.get_tuple3), '?ff', cb)

# Unit tests

client = new websock_client.KClient('127.0.0.1', 2)

exports.triggerServerBroadcast = (assert) ->
    assert.expect(3)

    assert.doesNotThrow( =>
        client.init( =>
            client.subscribeServerBroadcast( (channel, event_id) =>
                assert.equals(channel, 0)
                assert.equals(event_id, 0)
                client.exit()
                assert.done()
            )
            client.broadcastPing()
        )
    )

exports.sendManyParams = (assert) ->
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.sendManyParams(429496729, 2048, 3.14, true, (is_ok) =>
                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.readUint = (assert) ->
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readUint( (num) =>
                assert.equals(num, 301062138)
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
                assert.equals(num, -214748364)
                client.exit()
                assert.done()
            )
        )
    )

exports.readFloat = (assert) ->
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readFloat( (num) =>
                assert.ok(Math.abs(num - 3.141592) < 1e-7)
                client.exit()
                assert.done()
            )
        )
    )

exports.readDouble = (assert) ->
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readDouble( (num) =>
                assert.ok(Math.abs(num - 2.2250738585072009) < 1e-14)
                client.exit()
                assert.done()
            )
        )
    )

exports.setFloat = (assert) ->
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.setFloat( 12.5, (is_ok) =>
                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.rcvStdVector = (assert) ->
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.rcvStdVector( (array) =>
                is_ok = true
                for i in [0..array.length-1]
                    if array[i] != i*i*i
                        is_ok = false
                        break

                assert.ok(is_ok)
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

exports.rcvCArray1 = (assert) ->
    assert.expect(3)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            len = 10
            tests.rcvCArray1(len, (array) =>
                assert.equal(array.length, 2*len)
                is_ok = true
                for i in [0..2*len-1]
                    if array[i] != i/2
                        is_ok = false
                        break

                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.rcvCArray2 = (assert) ->
    assert.expect(3)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.rcvCArray2((array) =>
                assert.equal(array.length, 10)
                is_ok = true
                for i in [0..10-1]
                    if array[i] != i/4
                        is_ok = false
                        break

                assert.ok(is_ok)
                client.exit()
                assert.done()
            )
        )
    )

exports.sendBuffer = (assert) ->
    assert.expect(2)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            len = 10
            tests.sendBuffer(len, (is_ok) =>
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
    assert.expect(5)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readTuple( (tuple) =>
                assert.equals(tuple[0], 501762438)
                assert.ok(Math.abs(tuple[1] - 507.3858) < 5e-6)
                assert.ok(Math.abs(tuple[2] - 926547.6468507200) < 1e-14)
                assert.ok(tuple[3])
                client.exit()
                assert.done()
            )
        )
    )

exports.readTuple3 = (assert) ->
    assert.expect(4)

    assert.doesNotThrow( =>
        client.init( =>
            tests = new Tests(client)
            tests.readTuple3( (tuple) =>
                assert.ok(not tuple[0])
                assert.ok(Math.abs(tuple[1] - 3.14159) < 1e-6)
                assert.ok(Math.abs(tuple[2] - 507.3858) < 5e-6)
                client.exit()
                assert.done()
            )
        )
    )
