/// @file socket_interface.cpp
///
/// @brief Implementation of socket_interface.hpp
///
/// @author Thomas Vanderbruggen <thomas@koheron.com>
/// @date 15/08/2015
///
/// (c) Koheron 2014-2015

#include "socket_interface.hpp"

extern "C" {
  #include <arpa/inet.h>
}

#include "kserver_defs.hpp"

namespace kserver {

#define SEND_SPECIALIZE_IMPL(sock_interf)                           \
    template<>                                                      \
    int sock_interf::Send<std::string>(const std::string& str)      \
    {                                                               \
        return SendCstr(str.c_str());                               \
    }                                                               \
                                                                    \
    template<>                                                      \
    int sock_interf::Send<uint32_t>(const uint32_t& val)            \
    {                                                               \
        return SendArray<uint32_t>(&val, 1);                        \
    }                                                               \
                                                                    \
    template<>                                                      \
    int sock_interf::Send<uint64_t>(const uint64_t& val)            \
    {                                                               \
        return SendArray<uint64_t>(&val, 1);                        \
    }                                                               \
                                                                    \
    template<>                                                      \
    int sock_interf::Send<float>(const float& val)                  \
    {                                                               \
        return SendArray<float>(&val, 1);                           \
    }

// -----------------------------------------------
// TCP
// -----------------------------------------------

#if KSERVER_HAS_TCP

SEND_SPECIALIZE_IMPL(TCPSocketInterface)

int TCPSocketInterface::init() {return 0;}
int TCPSocketInterface::exit() {return 0;}

#define HEADER_LENGTH    12
#define HEADER_START     4  // First 4 bytes are reserved
#define HEADER_TYPE_LIST uint16_t, uint16_t, uint32_t

static_assert(
    required_buffer_size<HEADER_TYPE_LIST>() == HEADER_LENGTH - HEADER_START,
    "Unexpected header length"
);

int TCPSocketInterface::read_command(Command& cmd)
{
    // Read and decode header
    // |      RESERVED     | dev_id  |  op_id  |   payload_size    |   payload
    // |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | ...
    char header_buff[HEADER_LENGTH];
    bzero(header_buff, HEADER_LENGTH);
    int header_bytes = rcv_n_bytes(header_buff, HEADER_LENGTH);

    if (header_bytes <= 0)
        return header_bytes;

    auto header_tuple = parse_buffer<HEADER_START, HEADER_TYPE_LIST>(header_buff);

    uint32_t payload_size = std::get<2>(header_tuple);

    if (payload_size > CMD_PAYLOAD_BUFFER_LEN) {
        DEBUG_MSG("Receive command payload buffer size too small\n");
        return -1;
    }

    bzero(cmd.buffer, CMD_PAYLOAD_BUFFER_LEN);
    int payload_bytes = rcv_n_bytes(cmd.buffer, payload_size);

    if (payload_size > 0 && payload_bytes <= 0) 
        return payload_bytes;

    cmd.sess_id = id;
    cmd.device = static_cast<device_t>(std::get<0>(header_tuple));
    cmd.operation = std::get<1>(header_tuple);
    cmd.payload_size = payload_size;

    return header_bytes + payload_bytes;
}

int TCPSocketInterface::rcv_n_bytes(char *buffer, uint32_t n_bytes)
{
    if (n_bytes == 0)
        return 0;

    int bytes_rcv = 0;
    uint32_t bytes_read = 0;
    
    while (bytes_read < n_bytes) {
        bytes_rcv = read(comm_fd, buffer + bytes_read, n_bytes - bytes_read);
        
        if (bytes_rcv == 0) {
            fprintf(stderr, "Connection closed by client\n");
            return 0;
        }
        
        if (bytes_rcv < 0) {
            fprintf(stderr, "Can't receive data\n");
            return -1;
        }

        bytes_read += bytes_rcv;
        
        if (bytes_read >= KSERVER_READ_STR_LEN) {
            DEBUG_MSG("Buffer overflow\n");
            return -1;
        }
    }

    assert(bytes_read == n_bytes);
    return bytes_read;
}

int TCPSocketInterface::RcvDataBuffer(uint32_t n_bytes)
{
    uint32_t bytes_read = 0;

    while (bytes_read < n_bytes) {
        int result = read(comm_fd, recv_data_buff + bytes_read,
                          n_bytes-bytes_read);
        // Check reception ...
        if (result < -1) {
            kserver->syslog.print(SysLog::ERROR, "Read error\n");
            return -1;
        }

        if (result == 0) {
            kserver->syslog.print(SysLog::WARNING, 
                                  "Connection closed by client\n");
            return -1;
        }

        bytes_read += result;
        
        if (bytes_read > KSERVER_RECV_DATA_BUFF_LEN) {
            kserver->syslog.print(SysLog::CRITICAL, 
                                  "Receive data buffer overflow\n");
            return -1;
        }
    }

    assert(bytes_read == n_bytes);
    return bytes_read;
}

const uint32_t* TCPSocketInterface::RcvHandshake(uint32_t buff_size)
{
    // Handshaking
    if (Send<uint32_t>(htonl(buff_size)) < 0) {
        kserver->syslog.print(SysLog::ERROR, "Cannot send buffer size\n");
        return nullptr;
    }

    uint32_t n_bytes_received = 
        static_cast<uint32_t>(RcvDataBuffer(sizeof(uint32_t)*buff_size));
  
    if (n_bytes_received < 0)
        return nullptr;

    kserver->syslog.print(SysLog::DEBUG, "[R@%u] [%u bytes]\n", 
                          id, n_bytes_received);

    return reinterpret_cast<const uint32_t*>(recv_data_buff);
}

int TCPSocketInterface::SendCstr(const char *string)
{
    int bytes_send = strlen(string) + 1;
    int err = write(comm_fd, string, bytes_send);

    if (err < 0) {
        kserver->syslog.print(SysLog::ERROR, 
                              "SendCstr: Can't write to client\n");
        return -1;
    }

    return bytes_send;
}

#endif // KSERVER_HAS_TCP

// -----------------------------------------------
// WebSocket
// -----------------------------------------------

#if KSERVER_HAS_WEBSOCKET

SEND_SPECIALIZE_IMPL(WebSocketInterface)

int WebSocketInterface::init()
{    
    websock.set_id(comm_fd);

    if (websock.authenticate() < 0) {
        kserver->syslog.print(SysLog::CRITICAL, 
                              "Cannot connect websocket to client\n");	
        return -1;
    }

    return 0;
}

int WebSocketInterface::exit() {return 0;}

int WebSocketInterface::read_command(Command& cmd)
{
    if (websock.receive() < 0) { 
        if (websock.is_closed())
            return 0; // Connection closed by client
        else
            return -1;
    }

    if (websock.payload_size() < HEADER_LENGTH) {
        kserver->syslog.print(SysLog::ERROR, "Websocket: Command too small\n");
        return -1;
    }

    auto header_tuple = parse_buffer<HEADER_START, HEADER_TYPE_LIST>(websock.get_payload_no_copy());

    uint32_t payload_size = std::get<2>(header_tuple);

    if (payload_size > CMD_PAYLOAD_BUFFER_LEN) {
        DEBUG_MSG("Receive command payload buffer size too small\n");
        return -1;
    }

    if (payload_size + HEADER_LENGTH > websock.payload_size()) {
        kserver->syslog.print(SysLog::ERROR, "Websocket: Command payload reception incomplete\n");
        return -1;
    }

    if (payload_size + HEADER_LENGTH < websock.payload_size())
        kserver->syslog.print(SysLog::WARNING, "Websocket: Received more data than expected\n");


    bzero(cmd.buffer, CMD_PAYLOAD_BUFFER_LEN);
    memcpy(cmd.buffer, websock.get_payload_no_copy() + HEADER_LENGTH, payload_size);

    cmd.sess_id = id;
    cmd.device = static_cast<device_t>(std::get<0>(header_tuple));
    cmd.operation = std::get<1>(header_tuple);
    cmd.payload_size = payload_size;

    return HEADER_LENGTH + payload_size;
}

const uint32_t* WebSocketInterface::RcvHandshake(uint32_t buff_size)
{
    // Handshaking
    if (Send<uint32_t>(buff_size) < 0) {
        kserver->syslog.print(SysLog::ERROR, "Error sending the buffer size\n");
        return nullptr;
    }

    int payload_size = websock.receive();

    if (payload_size < 0)
        return nullptr;

    if (static_cast<uint32_t>(payload_size) != sizeof(uint32_t)*buff_size) {
        kserver->syslog.print(SysLog::ERROR, "Invalid data size received\n");
        return nullptr;
    }

    if (websock.get_payload(recv_data_buff, KSERVER_RECV_DATA_BUFF_LEN) < 0)
        return nullptr;

    return reinterpret_cast<const uint32_t*>(recv_data_buff);
}

int WebSocketInterface::SendCstr(const char *string)
{
    int err = websock.send(std::string(string));

    if (err < 0) {
        kserver->syslog.print(SysLog::ERROR, 
                              "SendCstr: Can't write to client\n");
        return -1;
    }

    return strlen(string) + 1;
}

#endif // KSERVER_HAS_WEBSOCKET

} // namespace kserver
