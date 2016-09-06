webclient = require('../lib/koheron-websocket-client.js')
Command = webclient.Command

class Math
    constructor : (@kclient) ->
        @device = @kclient.getDevice("Math")
        @cmds = @device.getCmds()

    add : (a, b, cb) ->
        @kclient.readUint32(Command(@device.id, @cmds.add, a, b), cb)

client = new webclient.KClient('127.0.0.1', 2)

client.init( =>
    math = new Math(client)
    math.add(1, 1, (res) ->
        console.log res
        process.exit()
    )
)
