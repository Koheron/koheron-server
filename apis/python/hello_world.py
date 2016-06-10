from koheron_tcp_client import KClient, command

class HelloWorld(object):
    def __init__(self, client):
        self.client = client

    @command('HELLO_WORLD','I')
    def add_42(self, num):
        return self.client.recv_uint32()

if __name__ == "__main__":
    client = KClient('127.0.0.1')
    hw = HelloWorld(client)
    print hw.add_42(58) # 100