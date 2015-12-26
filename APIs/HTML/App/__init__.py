import os
from flask import Flask, render_template, request, url_for
from koheron_tcp_client import KClient

class ServerApp(Flask):

    def __init__(self, *args, **kwargs):
        super(ServerApp, self).__init__(*args, **kwargs)
        self.client = KClient('127.0.0.1', verbose=False)
        

app = ServerApp(__name__)

from App import views

if __name__ == "__main__":
    app.run(host='127.0.0.1')
