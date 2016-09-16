from koheron import KoheronClient, command

class Math(object):
    def __init__(self, client):
        self.client = client

    @command()
    def add(self, a, b):
        return self.client.recv_uint32()

if __name__ == "__main__":
    client = KoheronClient('127.0.0.1')
    math = Math(client)
    assert math.add(1, 1) == 2
