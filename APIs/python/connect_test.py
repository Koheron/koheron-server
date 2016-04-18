from koheron_tcp_client import KClient, command

class Tests:
	def __init__(self, client):
		self.client = client

	@command('TESTS')
	def set_mean(self, mean): pass

client = KClient('127.0.0.1', 36100, verbose=True)
client.get_stats()

tests = Tests(client)
tests.set_mean(0.0)

# x = 10
# y = 20

# buff = bytearray()
# buff.append((x >> 8) & 0xff)
# buff.append(x & 0xff)
# buff.append((y >> 8) & 0xff)
# buff.append(y & 0xff)

# for char in buff:
#     print char