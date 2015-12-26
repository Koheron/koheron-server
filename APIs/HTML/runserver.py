#! /usr/bin/python

from App import app
import os

if __name__ == '__main__':
    port_ = int(os.getenv('FLASK_PORT', 5000))
    app.run(host='127.0.0.1', port=port_, debug=True)
