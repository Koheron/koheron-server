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
            set_float       : @device.getCmdRef( "SET_FLOAT"       )
            send_std_vector : @device.getCmdRef( "SEND_STD_VECTOR" )
            send_std_array  : @device.getCmdRef( "SEND_STD_ARRAY"  )
            send_c_array1   : @device.getCmdRef( "SEND_C_ARRAY1"   )
            send_c_array2   : @device.getCmdRef( "SEND_C_ARRAY2"   )
            set_buffer      : @device.getCmdRef( "SET_BUFFER"      )
            read_uint       : @device.getCmdRef( "READ_UINT"       )
            read_int        : @device.getCmdRef( "READ_INT"        )
            get_cstr        : @device.getCmdRef( "GET_CSTR"        )
            get_tuple       : @device.getCmdRef( "GET_TUPLE"       )

    setFloat : (f, cb) ->
        @kclient.readBool(Command(@id, @cmds.set_float, 'f', f), cb)

    rcvStdVector : (cb) ->
        @kclient.readFloat32Array(Command(@id, @cmds.send_std_vector), cb)

    rcvStdArray : (cb) ->
        @kclient.readFloat32Array(Command(@id, @cmds.send_std_array), cb)

    rcvCArray1 : (len, cb) ->
        @kclient.readFloat32Array(Command(@id, @cmds.send_c_array1, 'u', len), cb)

    rcvCArray2 : (cb) ->
        @kclient.readFloat32Array(Command(@id, @cmds.send_c_array2), cb)

    sendBuffer : (len, cb) ->
        buffer = new Uint32Array(len)
        (buffer[i] = i*i for i in [0..len])
        @kclient.sendArray(Command(@id, @cmds.set_buffer, 'u', buffer.length), buffer, (ok) -> cb(ok==1))

    checkBuffer : (cb) ->
        @kclient.readBool(Command(@id, @cmds.set_buffer), cb)

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