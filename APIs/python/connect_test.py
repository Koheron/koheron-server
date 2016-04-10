from koheron_tcp_client import KClient

client = KClient('127.0.0.1', 36100, verbose=True)

# x = 10
# y = 20

# buff = bytearray()
# buff.append((x >> 8) & 0xff)
# buff.append(x & 0xff)
# buff.append((y >> 8) & 0xff)
# buff.append(y & 0xff)

# for char in buff:
#     print char