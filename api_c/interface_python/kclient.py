#! /usr/bin/python

# @file kclient.py
#
# @brief Initializes connection to KServer
#
# Based on the C core library
#
# @author Thomas Vanderbruggen <thomas@koheron.com>
# @date 02/05/2015
#
# (c) Koheron 2015

import ctypes
import platform

class KClient:
    """ KServer client
    
    Initializes the connection with KServer, then retrieves 
    the current configuration: that is the available devices and
    the commands associated.
    
    It is also in charge of reception/emission of data with the KServer
    """
    
    def __init__(self, tcp_ip, tcp_port):
        """ Initialize connection with KServer
        
        Args:
            tcp_ip: A string with the IP address
            tcp_port: Port of the TCP connection (must be an integer)
            verbose: To display the retrieved KServer configuration
        """
        if type(tcp_ip) != str:
            raise TypeError("IP address must be a string")
           
        if type(tcp_port) != int:
            raise TypeError("Port number must be an integer")
    
        OS = platform.system()
    
        if OS == "Linux":
            self.libkoheron = ctypes.CDLL("./libkoheron.so")
        elif OS == "Windows":
            self.libkoheron = ctypes.CDLL("./libkoheron.dll")
        # TODO Add Mac support OS == "Darwin"
        else:
            raise RuntimeError("Unsuported operating system")
        
        # Prototype:
        # struct kclient * kclient_connect(char * host, int port)
        kclient_connect = self.libkoheron.kclient_connect
        kclient_connect.restype = ctypes.c_void_p
        
        self.kcl = kclient_connect(tcp_ip, tcp_port)
        
        if self.kcl == 0:
            raise RuntimeError("Cannot connect to KServer")
        
    def __del__(self):
        # Prototype:
        # void kclient_shutdown(struct kclient * kcl)
        kclient_shutdown = self.libkoheron.kclient_shutdown
        kclient_shutdown.argtypes = [ ctypes.c_void_p ]
        
        kclient_shutdown(self.kcl)
        
# ------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------

if __name__ == "__main__":
    IP = "10.42.0.58"
    PORT = 7
    
    client = KClient(IP, PORT)
