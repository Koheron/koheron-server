import time
import struct
import numpy as np

# ----------------------------------
# Receive
# ----------------------------------

def recv_n_bytes(sock, n_bytes):
    """ Receive exactly n bytes

    Args:
        sock: The socket used for communication
        n_bytes: Number of bytes to receive
    """

    data = []
    n_rcv = 0

    while n_rcv < n_bytes:
        try:
            chunk = sock.recv(n_bytes - n_rcv)

            if chunk == '':
                break

            n_rcv = n_rcv + len(chunk)
            data.append(chunk)
        except:
            print("recv_n_bytes: sock.recv failed")
            return ''

    return b''.join(data)


def recv_timeout(socket, escape_seq, timeout=5):
    """
    Receive data until either an escape sequence
    is found or a timeout is exceeded

    Args:
        socket: The socket used for communication
        escape_seq: String containing the escape sequence. Ex. 'EOF'.
        timeout: Reception timeout in seconds
    """

    # total data partwise in an array
    total_data = [];
    data = '';

    begin = time.time()

    while 1:
        if time.time()-begin > timeout:
            print("Error in recv_timeout Timeout exceeded")
            return "RECV_ERR_TIMEOUT"

        try:
            data = socket.recv(2048).decode('utf-8')

            if data:
                total_data.append(data)
                begin = time.time()

                if data.find(escape_seq) > 0:
                    break
        except:
            pass

        # To avoid the program to freeze at connection sometimes
        time.sleep(0.005)

    return ''.join(total_data)

# ----------------------------------
# Send
# ----------------------------------


def send_handshaking(sock, data, format_char='I'):
    """ Send data according to the handshaking protocol

    1) The size of the buffer must have been send as a
       command argument to KServer before
    2) KServer acknowledges reception readiness by sending
       the number of points to receive to the client
    3) The client send the data buffer

    Args:
        sock: The socket to use for communication
        data: The data buffer to be send
    """
    data_recv = sock.recv(4)
    num = struct.unpack(">I", data_recv)[0]
    n_pts = len(data)

    if num == n_pts:
        format_ = ('%s'+format_char) % n_pts
        buff = struct.pack(format_, *data.astype(np.int64))
        sent = sock.send(buff)

        if sent == 0:
            raise RuntimeError("Socket connection broken")
    else:
        raise RuntimeError("Invalid handshake")

    return