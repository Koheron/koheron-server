Koheron server Python API
==========================

Interface to initialize the connection with Koheron TCP server and to control the remote device memory.

Example, controlling the GPIOs on the Zynq (https://github.com/Koheron/zynq-sdk)
```py
from koheron_tcp_client import KClient, DevMem

host = '192.168.1.12'
password = 'changeme'

client = KClient(host)
dvm = DevMem(client)

# GPIO base address
GPIO_ADDR = int('0x41200000',0)
GPIO_SIZE = 65536

# GPIO registers offsets
GPIO_DATA  = 0
GPIO_TRI   = 4

# Open GPIO memory map
GPIO = dvm.add_memory_map(GPIO_ADDR, GPIO_SIZE)

# Set all GPIOs of channel 1 to output 
dvm.write(GPIO, GPIO_TRI, 0)

# Turn ON (3.3 V) the first 4 GPIOs of channel 1
val = 15
dvm.write(GPIO, GPIO_DATA, val)
assert dvm.read(GPIO, GPIO_DATA) == val
```

