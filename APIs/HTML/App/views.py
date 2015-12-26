from App import app
from flask import Flask, render_template

@app.route('/', methods=['GET'])
def index():
    return render_template('index.html')
    
@app.route('/server/status/devices/<device_name>', methods=['GET'])
def status_devices(device_name):
    try:
        params = app.client.cmds.get_device(device_name)
    except:
        print("Device not found")
        params = None

    return render_template('device.html', params=params)

if __name__ == "__main__":
    app.run(debug=True)
