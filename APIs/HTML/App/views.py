from App import app
from flask import Flask, render_template

@app.route('/', methods=['GET'])
def index():
    return render_template('index.html')
    
@app.route('/server/<int:device_id>/<int:command_id>', methods=['GET'])
def send_command():
    pass

if __name__ == "__main__":
    app.run(debug=True)
