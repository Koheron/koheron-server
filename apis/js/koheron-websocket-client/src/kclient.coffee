# Websocket based client for tcp-server
# (c) Koheron

class Device
    "use strict"

    constructor: (@devname, @id, @cmds) ->
        @status = ""

    show: ->
        console.log @devname + ":\n"
        console.log "  ID: " + @id + "\n"
        console.log "  Status: " + @status + "\n"
        console.log "  Commands:\n"
        (console.log "  - " + cmd + "\n" for cmd in @cmds)

    setStatus: (status) ->
        @status = status

    getCmdRef: (cmd_name) ->
        for cmd, idx in @cmds
            if cmd == cmd_name then return idx

        throw new ReferenceError(cmd_name + ': command not found')

    getCmds : ->
        cmds_dict = {}
        cmds_dict[cmd] = idx for cmd, idx in @cmds
        return cmds_dict

class WebSocketPool
    "use strict"

    constructor: (@pool_size, url, onopen_callback) ->
        if window? and not window.WebSocket then throw "WebSocket not supported"

        @pool = []
        @free_sockets = []
        @socket_counter = 0
        @exiting = false

        for i in [0..@pool_size-1]
            if window?
                websocket = new WebSocket url
            else # Node
                clientConfig = {}
                clientConfig.fragmentOutgoingMessages = false
                __WebSocket = require('websocket').w3cwebsocket
                websocket = new __WebSocket(url, null, null, null, null, clientConfig)

            websocket.binaryType = 'arraybuffer'
            @pool.push(websocket)

            websocket.onopen = (evt) =>
                @waitForConnection(websocket, 100, =>
                    if @exiting then return
                    # console.log "WebSocket " + @socket_counter.toString() + " connected to " + url + "\n"
                    console.assert(websocket.readyState == 1, "Websocket not ready")
                    @free_sockets.push(@socket_counter)
                    if @socket_counter == 0 then onopen_callback()
                    websocket.ID = @socket_counter
                    # console.log 'Socket ID = ' + websocket.ID.toString() + ' connected\n'

                    websocket.onclose = (evt) =>
                        # console.log 'Socket ID = ' + websocket.ID.toString() + ' disconnected\n'

                    websocket.onerror = (evt) =>
                        console.error "error: " + evt.data + "\n"
                        websocket.close()

                    @socket_counter++
                )

    waitForConnection: (websocket, interval, callback) ->
        if websocket.readyState == 1
            callback()
        else
            setTimeout( =>
                @waitForConnection(websocket, interval, callback)
            , interval)

    getSocket: (sockid) ->
        if @exiting then return null
        if sockid not in @free_sockets and sockid >= 0 and sockid < @pool_size
            return @pool[sockid]
        else
            throw new ReferenceError('Socket ID ' + sockid.toString() + ' not available')

    requestSocket: (callback) ->
        if @exiting
            callback(-1)
            return

        if @free_sockets? and @free_sockets.length > 0
            sockid = @free_sockets[@free_sockets.length-1]
            @free_sockets.pop()
            @waitForConnection(@pool[sockid], 100, =>
                console.assert(@pool[sockid].readyState == 1, "Websocket not ready")
                callback(sockid)
            )
        else # All sockets are busy. We wait a bit and retry.
            console.assert(@free_sockets.length == 0, 'Non empty free_sockets')
            setTimeout( =>
                @requestSocket(callback)
            , 100)

    freeSocket: (sockid) ->
        if @exiting
            return
        if sockid not in @free_sockets and sockid >= 0 and sockid < @pool_size
            @free_sockets.push(sockid)
        else
            if sockid?
                console.error "Invalid Socket ID " + sockid.toString()
            else
                console.error "Undefined Socket ID"

    exit: ->
        @exiting = true
        @free_sockets = []
        @socket_counter = 0

        for websocket in @pool
            websocket.close()

# === Helper functions to build binary buffer ===

###*
# Append an Int8 to the binary buffer
# @param {Array.<number>} buffer The binary buffer
# @param {number} value The Int8 to append
# @return {number} Number of bytes
###
appendInt8 = (buffer, value) ->
    buffer.push(value & 0xff)
    return 1

###*
# Append an Uint8 to the binary buffer
# @param {Array.<number>} buffer The binary buffer
# @param {number} value The Uint8 to append
# @return {number} Number of bytes
###
appendUint8 = (buffer, value) ->
    buffer.push(value & 0xff)
    return 1

###*
# Append an Int16 to the binary buffer
# @param {Array.<number>} buffer The binary buffer
# @param {number} value The Int16 to append
# @return {number} Number of bytes
###
appendInt16 = (buffer, value) ->
    buffer.push((value >> 8) & 0xff)
    buffer.push(value & 0xff)
    return 2

###*
# Append an Uint16 to the binary buffer
# @param {Array.<number>} buffer The binary buffer
# @param {number} value The Uint16 to append
# @return {number} Number of bytes
###
appendUint16 = (buffer, value) ->
    buffer.push((value >> 8) & 0xff)
    buffer.push(value & 0xff)
    return 2

###*
# Append an Int32 to the binary buffer
# @param {Array.<number>} buffer The binary buffer
# @param {number} value The Int32 to append
# @return {number} Number of bytes
###
appendInt32 = (buffer, value) ->
    buffer.push((value >> 24) & 0xff)
    buffer.push((value >> 16) & 0xff)
    buffer.push((value >> 8) & 0xff)
    buffer.push(value & 0xff)
    return 4

###*
# Append an Uint32 to the binary buffer
# @param {Array.<number>} buffer The binary buffer
# @param {number} value The Uint32 to append
# @return {number} Number of bytes
###
appendUint32 = (buffer, value) ->
    buffer.push((value >> 24) & 0xff)
    buffer.push((value >> 16) & 0xff)
    buffer.push((value >> 8) & 0xff)
    buffer.push(value & 0xff)
    return 4

###*
# Convert a float to its binary representation
# http://stackoverflow.com/questions/2003493/javascript-float-from-to-bits
# @param {number} f The float to convert
# @return {number} Uint32 containing the float binary representation
###
floatToBytes = (f) ->
    buf = new ArrayBuffer(4)
    (new Float32Array(buf))[0] = f
    return (new Uint32Array(buf))[0]

bytesTofloat = (bytes) ->
    buffer = new ArrayBuffer(4)
    (new Uint32Array(buffer))[0] = bytes
    return new Float32Array(buffer)[0]

###*
# Append a Float32 to the binary buffer
# @param {Array.<number>} buffer The binary buffer
# @param {number} value The float to append
# @return {number} Number of bytes
###
appendFloat32 = (buffer, value) ->
    appendUint32(buffer, floatToBytes(value))

###*
# Append a Float64 to the binary buffer
# @param {Array.<number>} buffer The binary buffer
# @param {number} value The double to append
# @return {number} Number of bytes
###
appendFloat64 = (buffer, value) ->
    buf = new ArrayBuffer(8)
    (new Float64Array(buf))[0] = value
    appendUint32(buffer, (new Uint32Array(buf, 0))[0])
    appendUint32(buffer, (new Uint32Array(buf, 4))[0])
    console.assert(buf.byteLength == 8, "Invalid float64 size")
    return buf.byteLength

###*
# Append an Array to the binary buffer
# @param {Array.<number>} buffer The binary buffer
# @param {Array.<number>} array The array to append
# @return {number} Number of bytes
###
appendArray = (buffer, array) ->
    bytes = new Uint8Array(array.buffer)
    buffer.push(_byte) for _byte in bytes
    return array.buffer.byteLength

class CommandBase
    "use strict"

    # Binary buffer structure:
    # |      RESERVED     | dev_id  |  op_id  |   payload_size    |   payload
    # |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | ...

    # http://stackoverflow.com/questions/27050784/closure-annotation-for-variadic-function
    ###*
    # Build the binary buffer of a command
    # @type {function(number, string=, ...(number|boolean)):Uint8Array}
    ###
    constructor: (dev_id, cmd_ref, types_str='', params...) ->
        buffer = []
        appendUint32(buffer, 0) # RESERVED
        appendUint16(buffer, dev_id)
        appendUint16(buffer, cmd_ref)

        if types_str.length == 0
            appendUint32(buffer, 0) # Payload size
            return new Uint8Array(buffer)

        if types_str.length != params.length
            throw new Error('Invalid types string length')

        payload_size = 0
        payload = []
        for i in [0..(types_str.length-1)]
            switch types_str[i]
                when 'B'
                    payload_size += appendUint8(payload, params[i])
                when 'b'
                    payload_size += appendInt8(payload, params[i])
                when 'H'
                    payload_size += appendUint16(payload, params[i])
                when 'h'
                    payload_size += appendInt16(payload, params[i])
                when 'I'
                    payload_size += appendUint32(payload, params[i])
                when 'i'
                    payload_size += appendInt32(payload, params[i])
                when 'f'
                    payload_size += appendFloat32(payload, params[i])
                when 'd'
                    payload_size += appendFloat64(payload, params[i])
                when '?'
                    if params[i]
                        payload_size += appendUint8(payload, 1)
                    else
                        payload_size += appendUint8(payload, 0)
                when 'A'
                    payload_size += appendArray(payload, params[i])
                else
                    throw new TypeError('Unknown type ' + types_str[i])

        appendUint32(buffer, payload_size)
        return new Uint8Array(buffer.concat(payload))

###* @param {...*} var_args ###
Command = (var_args) ->
    CommandBase.apply(Object.create(Command.prototype), arguments)

if window? # Browser
    window.Command = Command
else # NodeJS
    exports.Command = Command

class @KClient
    "use strict"

    constructor: (IP, @websock_pool_size = 5) ->
        @url = "ws://" + IP + ":8080"
        @devices_list = []
        @broadcast_socketid = -1

    init: (callback) ->
        @websockpool = new WebSocketPool(@websock_pool_size, @url, => 
            @loadCmds(callback)
        )

    subscribeServerBroadcast: (callback) ->
        @websockpool.requestSocket( (sockid) =>
            if sockid < 0 then return callback(null, null)
            @broadcast_socketid = sockid
            broadcast_socket = @websockpool.getSocket(sockid)
            broadcast_socket.send(Command(1, 5, 'I', 0)) # Server broadcasts on channel 0

            broadcast_socket.onmessage = (evt) =>
                tup = @deserialize('III', evt.data)
                reserved = tup[0]
                console.assert(reserved == 0, 'Non-zero event message reserved bytes')
                channel = tup[1]
                event_id = tup[2]
                callback(channel, event_id)
        )

    broadcastPing: ->
        if not @websockpool? then return

        @websockpool.requestSocket( (sockid) =>
            if sockid < 0 then return
            websocket = @websockpool.getSocket(sockid)
            websocket.send(Command(1, 6))
            if @websockpool?
                @websockpool.freeSocket(sockid)
        )

    exit: ->
        @websockpool.exit()
        delete @websockpool

    # ------------------------
    #  Send
    # ------------------------

    send: (cmd) ->
        if not @websockpool? then return

        @websockpool.requestSocket( (sockid) =>
            if sockid < 0 then return
            websocket = @websockpool.getSocket(sockid)
            websocket.send(cmd)
            if @websockpool?
                 @websockpool.freeSocket(sockid)
        )

    ###*
    # Send an array
    # @param {Uint8Array} cmd - The command containing the array length
    # @param {
    #          Array.<number>|
    #          Uint32Array|Int32Array|
    #          Uint16Array|Int16Array|
    #          Uint8Array |Int8Array |
    #          Float32Array|Float64Array
    #        } array - The array to be send
    # @param {function(*)|null} cb - Optionnal callback if the command receiving the 
    #     array sends data back (must be of interger type).
    ###
    sendArray: (cmd, array, cb=null) ->
        @websockpool.requestSocket( (sockid) =>
            websocket = @websockpool.getSocket(sockid)
            websocket.send(cmd)

            websocket.onmessage = (evt) =>
                tmp_array = new Uint32Array evt.data
                if tmp_array[0] != array.length
                    throw new Error('Invalid handshake')
                websocket.send(array.buffer)

                if cb != null
                    websocket.onmessage = (evt) =>
                        array = new Uint32Array evt.data
                        cb(array[0])
                        if @websockpool?
                            @websockpool.freeSocket(sockid)
                else
                    if @websockpool?
                        @websockpool.freeSocket(sockid)
        )

    # ------------------------
    #  Receive
    # ------------------------

    _readBase: (cmd, fn) ->
        if not @websockpool? then return fn(null)

        @websockpool.requestSocket( (sockid) =>
            if sockid < 0 then return fn(null)
            websocket = @websockpool.getSocket(sockid)
            websocket.send(cmd)

            websocket.onmessage = (evt) =>
                fn(evt.data)
                if @websockpool?
                    @websockpool.freeSocket(sockid)
        )

    readUint32Array: (cmd, fn) ->
        @_readBase(cmd, (data) =>
            array = new Uint32Array data
            fn(array)
        )

    readFloat32Array: (cmd, fn) ->
        @_readBase(cmd, (data) =>
            array = new Float32Array data
            fn(array)
        )

    readUint32: (cmd, fn) ->
        @_readBase(cmd, (data) =>
            array = new Uint32Array data
            fn(array[0])
        )

    readInt32: (cmd, fn) ->
        @_readBase(cmd, (data) =>
            array = new Int32Array data
            fn(array[0])
        )

    readFloat32: (cmd, fn) ->
        @_readBase(cmd, (data) =>
            array = new Float32Array data
            fn(array[0])
        )

    readFloat64: (cmd, fn) ->
        @_readBase(cmd, (data) =>
            array = new Float64Array data
            fn(array[0])
        )

    readBool: (cmd, fn) ->
        @readUint32(cmd, (num) => fn(num == 1))

    readTuple: (cmd, fmt, fn) ->
        @_readBase(cmd, (data) => fn(@deserialize(fmt, data)))

    deserialize: (fmt, data) ->
        dv = new DataView(data)
        tuple = []
        offset = 0

        for i in [0..(fmt.length-1)]
            switch fmt[i]
                when 'B'
                    tuple.push(dv.getUint8(offset))
                    offset += 1
                when 'b'
                    tuple.push(dv.getInt8(offset))
                    offset += 1
                when 'H'
                    tuple.push(dv.getUint16(offset))
                    offset += 2
                when 'h'
                    tuple.push(dv.getInt16(offset))
                    offset += 2
                when 'I'
                    tuple.push(dv.getUint32(offset))
                    offset += 4
                when 'i'
                    tuple.push(dv.getInt32(offset))
                    offset += 4
                when 'f'
                    tuple.push(dv.getFloat32(offset))
                    offset += 4
                when 'd'
                    tuple.push(dv.getFloat64(offset))
                    offset += 8
                when '?'
                    if dv.getUint8(offset) == 0
                        tuple.push(false)
                    else
                        tuple.push(true)
                    offset += 1
                else
                    throw new TypeError('Unknown or unsupported type ' + fmt[i])

        return tuple

    readString: (cmd, fn) ->
        @_readBase(cmd, (data) =>
            dv = new DataView(data)
            reserved = dv.getUint32(0)
            len = dv.getUint32(4)
            chars = []
            chars.push(String.fromCharCode(dv.getUint8(8 + i))) for i in [0..len-2]
            fn(chars.join(''))
        )

    readJSON: (cmd, fn) ->
        @readString(cmd, (str) => fn(JSON.parse(str)))

    # ------------------------
    #  Devices
    # ------------------------

    loadCmds: (callback) ->
        @readJSON(Command(1,1), (data) =>
            for dev, id in data
                dev = new Device(dev.name, id, dev.operations)
                # dev.show()
                @devices_list.push(dev)

            callback()
        )

    getDeviceList: (fn) ->
        @getDevStatus( () => fn(@devices_list) )

    showDevices: ->
        console.log "Devices:\n"
        (device.show() for device in @devices_list)

    getDevice: (devname) ->
        for device in @devices_list
            if device.devname == devname then return device

        throw new Error('Device ' + devname + ' not found')

    getDeviceById: (id) ->
        for device in @devices_list
            if device.id == id then return device

        throw new ReferenceError('Device ID ' + id + ' not found')
