#! /usr/bin/python

# Device memory access
#
# Based on the C core library
#
# (c) Koheron

import ctypes

class Devmem:
    def __init__(self, client):
        """ Start the device memory manager
        
        Args:
            client The Kclient object
        """
        self.libkoheron = client.libkoheron
        
        # Prototype:
        # struct devmem * dev_mem_init(struct kclient * kcl)
        dev_mem_init = self.libkoheron.dev_mem_init
        dev_mem_init.restype = ctypes.c_void_p
        dev_mem_init.argtypes = [ ctypes.c_void_p ]
        
        self.dvm = dev_mem_init(client.kcl)
        
        if self.dvm == 0:
            raise RuntimeError("Cannot start Devmem device")
            
    def __del__(self):
        # Prototype:
        # void dev_mem_exit(struct devmem * dvm)
        dev_mem_exit = self.libkoheron.dev_mem_exit
        dev_mem_exit.argtypes = [ ctypes.c_void_p ]
        dev_mem_exit(self.dvm)
        
# ------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------

if __name__ == "__main__":
    import kclient
    
    IP = "10.42.0.58"
    PORT = 7
    
    client = kclient.KClient(IP, PORT)

    dvm = Devmem(client)
