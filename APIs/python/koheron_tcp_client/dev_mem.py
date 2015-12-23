# dev_mem.py
# Client API for the DEV_MEM device

import math
from .kclient import reference_dict

class DevMem:
    """
    Map and access physical memory
    """
    def __init__(self, client):
        self.client = client

        # Dictionnary of references
        self.ref = reference_dict(self)

        self.open()
        return

    def open(self):
        self.client.send_command(self.ref['id'], self.ref['open'])

    def add_memory_map(self, device_addr, map_size):
        """ Add a memory map

        Args:
            device_addr: Physical address of the device
            map_size: Mmap size

        Return:
            The ID of the memory map
        """
        self.client.send_command(self.ref['id'],
                                 self.ref['add_memory_map'],
                                 device_addr, map_size)

        map_id = self.client.recv_int(4, 'ERR')

        if math.isnan(map_id):
            print("Can't open memory map")

        return map_id


    def rm_memory_map(self, mmap_idx):
        """ Remove a memory map

        Args:
            mmap_idx: Index of Memory Map
        """
        self.client.send_command(self.ref['id'],
                                 self.ref['rm_memory_map'],
                                 mmap_idx)

    def read(self, mmap_idx, offset):
        """ Read a register

        Args:
            mmap_idx: Index of Memory Map
            offset: Offset of the register to read

        Return:
            The value of the register (integer)
        """
        self.client.send_command(self.ref['id'],
                                 self.ref['read'],
                                 mmap_idx, offset)
        return self.client.recv_int(4)

    def write(self, mmap_idx, offset, reg_val):
        """ Write a register

        Args:
            mmap_idx: Index of Memory Map
            offset: Offset of the register to read
            reg_val: Value to write in the register
        """
        self.client.send_command(self.ref['id'],
                                 self.ref['write'],
                                 mmap_idx, offset, reg_val)

    def write_buffer(self, mmap_idx, offset, data):
        """ Write a buffer of registers

        Args:
            mmap_idx: Index of Memory Map
            offset: Offset of the register to write
            len_data: Size of the buffer data. To be used for the handshaking.
        """
        len_data = len(data)
        self.client.send_command(self.ref['id'],
                                 self.ref['write_buffer'],
                                 mmap_idx, offset, len_data)
        self.client.send_handshaking(data)

    def read_buffer(self, mmap_idx, offset, buff_size, data_type='uint32'):
        """ Read a buffer of registers

        Args:
            mmap_idx: Index of Memory Map
            offset: Offset of the register to read
            buff_size: Number of registers to read
            data_type: Type of received data

        Returns:
            A Numpy array with the data on success.
            A Numpy array of NaNs on failure.
        """
        self.client.send_command(self.ref['id'],
                                 self.ref['read_buffer'],
                                 mmap_idx, offset, buff_size)
        return self.client.recv_buffer(buff_size, data_type)

    def set_bit(self, mmap_idx, offset, index):
        """ Set a bit (bit = 1)

        Args:
            mmap_idx: Index of Memory Map
            offset: Offset of the register
            index: Index of the bit to set in the register
        """
        self.client.send_command(self.ref['id'],
                                 self.ref['set_bit'],
                                 mmap_idx, offset, index)

    def clear_bit(self, mmap_idx, offset, index):
        """ Clear a bit (bit = 0)

        Args:
            mmap_idx: Index of Memory Map
            offset: Offset of the register
            index: Index of the bit to cleared in the register
        """
        self.client.send_command(self.ref['id'],
                                 self.ref['clear_bit'],
                                 mmap_idx, offset, index)

    def toggle_bit(self, mmap_idx, offset, index):
        """ Toggle the value of a bit

        Args:
            mmap_idx: Index of Memory Map
            offset: Offset of the register
            index: Index of the bit to set in the register
        """
        self.client.send_command(self.ref['id'],
                                 self.ref['toggle_bit'],
                                 mmap_idx, offset, index)
