# kclient.py
# Initializes connection to KServer
# Thomas Vanderbruggen <thomas@koheron.com>
# (c) Koheron 2015

import socket
import struct
from rcv_send import recv_timeout, recv_n_bytes, recv_buffer, send_handshaking

# --------------------------------------------
# Helper functions
# --------------------------------------------

def make_command(*args):
    return "|".join([str(arg) for arg in args])+'|\n'

# TODO
# This approach implies some strong constraints on the class and 
# method naming. This should be referenced in some doc.

def reference_dict(self):
    params = self.client.cmds.get_device(_class_to_device_name(self.__class__.__name__))
    ref_dict = {'id': str(params.id)}
    for method in dir(self):
        if callable(getattr(self, method)):
            op_ref = params.get_op_ref(method.upper())
            if op_ref >= 0:
                ref_dict[method] = str(params.get_op_ref(method.upper()))
    return ref_dict

def _class_to_device_name(classname):
    """
    If the device name is in a single word DEVNAME then the associated 
    class name must be Devname.
    
    If the device name is in a several words DEV_NAME then the associated 
    class name must be DevName.
    """
    dev_name = []
        
    # Check whether there are capital letters within the class name
    # and insert an underscore before them
    for idx, letter in enumerate(classname):
        if idx > 0 and letter == letter.upper():
            dev_name.append('_')
                
        dev_name.append(letter.upper())
            
    return ''.join(dev_name)


# --------------------------------------------
# KClient
# --------------------------------------------


class KClient:
    """ KServer client
    
    Initializes the connection with KServer, then retrieves 
    the current configuration: that is the available devices and
    the commands associated.
    
    It is also in charge of reception/emission of data with KServer
    """
    
    def __init__(self, host, port=36000, verbose = False, timeout=1.0):
        """ Initialize connection with KServer
        
        Args:
            host: A string with the IP address
            port: Port of the TCP connection (must be an integer)
            verbose: To display the retrieved KServer configuration
        """
        if type(host) != str:
            raise TypeError("IP address must be a string")
           
        if type(port) != int:
            raise TypeError("Port number must be an integer")
            
        self.verbose = verbose
        self.is_connected = False

        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            #self.sock.settimeout(timeout)
            #   Disable Nagle algorithm for real-time response:
            self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            # Connect to Kserver
            self.sock.connect((host, port))
            self.is_connected = True
            
        except socket.error as e:
            print 'Failed to connect to {:s}:{:d} : {:s}'.format(host, port, e)

        # If connected get the commands
        if self.is_connected:
            self.cmds = Commands(self)
        
    # -------------------------------------------------------
    # Send/Receive
    # -------------------------------------------------------
    
    def send(self,cmd):
        """ Send a command
        
        Args:
            cmd: The command to be send
        """
        sent = self.sock.send(cmd)
        
        if sent == 0:
            raise RuntimeError("Socket connection broken")

    def send_command(self, device_id, operation_ref, *args):
        self.send(make_command(device_id, operation_ref, *args))
        
    def recv_int(self, buff_size, err_msg = None):
        """ Receive an integer
        
        Args:
            buff_size: Maximum amount of data to be received at once
            err_msg: Error message. If you require the server to 
                     send an message signaling an error occured and 
                     that no data can be retrieve.
        """
        data_recv = self.sock.recv(buff_size)
        
        if data_recv == '':
            raise RuntimeError("Socket connection broken")
            
        if err_msg != None:
            if data_recv[:len(err_msg)] == err_msg:
                raise RuntimeError("No data available")
            
        return struct.unpack("I", data_recv)[0]
        
    def recv_n_bytes(self, n_bytes):
        """ Receive exactly n bytes
    
        Args:
            n_bytes: Number of bytes to receive
        """
        return recv_n_bytes(self.sock, n_bytes)
        
    def recv_buffer(self, buff_size, data_type = 'uint32'):
        """ Receive a buffer of uint32
        
        Args:
            buff_size Number of samples to receive
        """
        return recv_buffer(self.sock, buff_size, data_type=data_type)
        
    def recv_tuple(self):
        tmp_buffer = [' ']
        res_tuple = []
        
        while tmp_buffer[-1] != '\n':
            char = recv_n_bytes(self.sock, 1)
            
            if char == ':':
                if tmp_buffer[1] == '\x00':
                    toks = ''.join(tmp_buffer[2:]).split('@')
                else:
                    toks = ''.join(tmp_buffer[1:]).split('@')
                elmt_type = toks[0].strip()
                
                if elmt_type == 'i' or elmt_type == 'j':
                    res_tuple.append(int(toks[1]))
                elif elmt_type == 'f':
                    res_tuple.append(float(toks[1]))
                else: # String
                    res_tuple.append(toks[1])
                    
                tmp_buffer = [' ']
            else:
                tmp_buffer.append(char)
            
        return tuple(res_tuple)
        
    def send_handshaking(self, data, format_char='I'):
        """ Send data with handshaking protocol
        
        Args:
            data: The data buffer to be sent
            format_char: format character, unsigned int by default (see https://docs.python.org/2/library/struct.html#format-characters)
        """
        send_handshaking(self.sock, data, format_char=format_char)
        
    # -------------------------------------------------------
    # Current session information
    # -------------------------------------------------------
        
    def get_session_status(self):
        """ Status of the current session
        
        Return: True if an error occured in the current session.
        """
        sent = self.send(make_command(1,2))
        data_recv = self.sock.recv(3)
        
        if data_recv == '':
            raise RuntimeError("Socket connection broken")
            
        if data_recv[:2] == 'OK':
            return True
        else:
            return False
            
    def get_devices_status(self):
        """ Status of the devices
        """
        # TODO
        return
        
    def get_server_log(self):
        """ Return KServer log
        
        Return: 
            An array of the commands executed by KServer 
            during the current session.
            
            Each log entry is a dictionnary containing:
                * "Command#": Number of the executed command
                * "Device": Execution device
                * "Operation": Operation run on the device
                * "Parsing": Command parsing status
                * "Status": Execution status
        """
        return KserverLog(self)
        
    def __del__(self):
        if hasattr(self,'sock'):
            self.sock.close()
           
class Commands:
    """ KServer commands
    
    Retrieves and stores the commands (devices and 
    associated operations) available in KServer.
    """ 
    def __init__(self, client):
        """ Receive and parse the commands description message sent by KServer
        """
        sent = client.sock.send(make_command(1,1))        
        
        if sent == 0:
            raise RuntimeError("Socket connection broken")
            
            
        msg = recv_timeout(client.sock, 'EOC')
        lines = msg.split('\n')

        self.devices = []
        
        for line in lines[1:-2]:
            self.devices.append(DevParam(line))
            
        if client.verbose:
            self.print_devices()
        
    def get_device(self, device_name):
        """ Provide device parameters
        """    
        for device in self.devices:
            if device.name == device_name:
                return device
        
        raise ValueError('Device ' + device_name + ' unknown')
        
    def print_devices(self):
        """ Print the devices and operations available on KServer
        """
        print "Devices available from KServer:"
        
        for device in self.devices:      
            device.show()
            
class DevParam:
    """ Device parameters
    
    Store the parameters related to a devices: 
    the device name and its ID, together with 
    the associated operations and their reference
    """
    
    def __init__(self, line):
        """
        Parse device informations sent by KServer
        """
        tokens = line.split(':')
        self.id = int(tokens[0][1:])
        self.name = tokens[1].strip()
        
        self.operations = []
        op_num = 0
        
        for tok in tokens[2:]:
            if len(tok.strip()) != 0:
                self.operations.append(tok.strip())
                op_num = op_num + 1
            
        self.op_num = op_num
        
    def get_op_ref(self, op_name):
        """ Return the reference of a given operation
        
        Args:
            op_name: Name of the operation
        """
        try:
            return self.operations.index(op_name)
        except:
            return -1
            #raise ValueError('Operation ' + op_name + ' is unknown for device ' + self.name)
            
    def show(self):
        """ Display the device parameters
        """
        print '\n> ' + self.name
        print 'ID: ' + str(self.id)
        
        print 'Operations:'
        for idx, op in enumerate(self.operations):
            print '  ' + op + '(' + str(idx) + ')'

