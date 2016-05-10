from koheron_tcp_client import KClient, command

class Tests:
	def __init__(self, client):
		self.client = client

	@command('TESTS')
	def set_mean(self, mean): pass

	@command('TESTS')
	def send_std_array(self):
		return client.recv_buffer(10, data_type='float32')


client = KClient('127.0.0.1', 36000, verbose=True)
client.get_stats()

tests = Tests(client)
tests.set_mean(12.5)
print tests.send_std_array()
