/// (c) Koheron

#include "kserver_session.hpp"

namespace kserver {

const uint32_t* SessionAbstract::RcvHandshake(uint32_t buff_size)
{
    SWITCH_SOCK_TYPE(RcvHandshake(buff_size))
    return nullptr;
}

int SessionAbstract::SendCstr(const char *string)
{
    SWITCH_SOCK_TYPE(SendCstr(string))
    return -1;
}

#define SEND_SPECIALIZE_IMPL(session_kind)                          \
    template<> template<>                                           \
    int session_kind::Send<std::string>(const std::string& str)     \
    {                                                               \
        return SendCstr(str.c_str());                               \
    }                                                               \
                                                                    \
    template<> template<>                                           \
    int session_kind::Send<uint32_t>(const uint32_t& val)           \
    {                                                               \
        return SendArray<uint32_t>(&val, 1);                        \
    }                                                               \
                                                                    \
    template<> template<>                                           \
    int session_kind::Send<uint64_t>(const uint64_t& val)           \
    {                                                               \
        return SendArray<uint64_t>(&val, 1);                        \
    }                                                               \
                                                                    \
    template<> template<>                                           \
    int session_kind::Send<float>(const float& val)                 \
    {                                                               \
        return SendArray<float>(&val, 1);                           \
    }

// -----------------------------------------------
// TCP
// -----------------------------------------------

#if KSERVER_HAS_TCP

SEND_SPECIALIZE_IMPL(Session<TCP>)

template<> int Session<TCP>::init_socket() {return 0;}
template<> int Session<TCP>::exit_socket() {return 0;}

#define HEADER_LENGTH    12
#define HEADER_START     4  // First 4 bytes are reserved
#define HEADER_TYPE_LIST uint16_t, uint16_t, uint32_t

static_assert(
    required_buffer_size<HEADER_TYPE_LIST>() == HEADER_LENGTH - HEADER_START,
    "Unexpected header length"
);

template<>
int Session<TCP>::read_command(Command& cmd)
{
    // Read and decode header
    // |      RESERVED     | dev_id  |  op_id  |   payload_size    |   payload
    // |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | ...
    Buffer<HEADER_LENGTH> header_buff;
    int header_bytes = rcv_n_bytes(header_buff, HEADER_LENGTH);

    if (header_bytes == 0)
        return header_bytes;

    if (header_bytes < 0) {
        session_manager.kserver.syslog.print(SysLog::ERROR,
            "TCPSocket: Cannot read header\n");
        return header_bytes;
    }

    auto header_tuple
        = parse_buffer<HEADER_START, header_buff.size(), HEADER_TYPE_LIST>(header_buff);
    uint32_t payload_size = std::get<2>(header_tuple);

    cmd.sess_id = id;
    cmd.device = static_cast<device_t>(std::get<0>(header_tuple));
    cmd.operation = std::get<1>(header_tuple);
    cmd.payload_size = payload_size;

    if (payload_size > cmd.buffer.size()) {
        session_manager.kserver.syslog.print(SysLog::ERROR, 
                  "TCPSocket: Command payload buffer size too small\n"
                  "payload_size = %u cmd.buffer.size = %u\n",
                  payload_size, cmd.buffer.size());
        return -1;
    }

    int payload_bytes = 0;

    if (payload_size > 0) {
        payload_bytes = rcv_n_bytes(cmd.buffer, payload_size);

        if (payload_bytes == 0)
            return payload_bytes;

        if (payload_bytes < 0) {
            session_manager.kserver.syslog.print(SysLog::ERROR, 
                "TCPSocket: Cannot read payload for device %u, operation %u\n", 
                cmd.device, cmd.operation);
            return header_bytes;
        }
    }

    session_manager.kserver.syslog.print(SysLog::DEBUG, 
        "TCPSocket: Receive command for device %u, operation %u [%u bytes]\n", 
        cmd.device, cmd.operation, cmd.payload_size);

    return header_bytes + payload_bytes;
}

template<>
template<size_t len>
int Session<TCP>::rcv_n_bytes(Buffer<len>& buffer, uint32_t n_bytes)
{
    assert(n_bytes <= len);

    if (n_bytes == 0)
        return 0;

    int bytes_rcv = 0;
    uint32_t bytes_read = 0;

    while (bytes_read < n_bytes) {
        bytes_rcv = read(comm_fd, buffer.data + bytes_read, n_bytes - bytes_read);
        
        if (bytes_rcv == 0) {
            session_manager.kserver.syslog.print(SysLog::INFO,
                "TCPSocket: Connection closed by client\n");
            return 0;
        }

        if (bytes_rcv < 0) {
            session_manager.kserver.syslog.print(SysLog::ERROR,
                "TCPSocket: Can't receive data\n");
            return -1;
        }

        bytes_read += bytes_rcv;
        assert(bytes_read <= len);
    }

    assert(bytes_read == n_bytes);
    session_manager.kserver.syslog.print(SysLog::DEBUG, 
                "[R@%u] [%u bytes]\n", id, bytes_read);

    return bytes_read;
}

template<>
const uint32_t* Session<TCP>::RcvHandshake(uint32_t buff_size)
{
    // Handshaking
    if (Send<uint32_t>(htonl(buff_size)) < 0) {
        session_manager.kserver.syslog.print(SysLog::ERROR,
            "TCPSocket: Cannot send buffer size\n");
        return nullptr;
    }

    int n_bytes_received = rcv_n_bytes(recv_data_buff, sizeof(uint32_t) * buff_size);

    if (n_bytes_received < 0)
        return nullptr;

    session_manager.kserver.syslog.print(SysLog::DEBUG, "[R@%u] [%u bytes]\n", 
                          id, n_bytes_received);

    return reinterpret_cast<const uint32_t*>(recv_data_buff.data);
}

template<>
int Session<TCP>::SendCstr(const char *string)
{
    int bytes_send = strlen(string) + 1;
    int err = write(comm_fd, string, bytes_send);

    if (err < 0) {
        session_manager.kserver.syslog.print(SysLog::ERROR, 
                              "TCPSocket::SendCstr: Can't write to client\n");
        return -1;
    }

    return bytes_send;
}

#endif

// -----------------------------------------------
// WebSocket
// -----------------------------------------------

#if KSERVER_HAS_WEBSOCKET

SEND_SPECIALIZE_IMPL(Session<WEBSOCK>)

template<>
int Session<WEBSOCK>::init_socket()
{
    websock.set_id(comm_fd);

    if (websock.authenticate() < 0) {
        session_manager.kserver.syslog.print(SysLog::CRITICAL, 
                              "Cannot connect websocket to client\n");  
        return -1;
    }

    return 0;
}

template<> int Session<WEBSOCK>::exit_socket() {return 0;}

template<>
int Session<WEBSOCK>::read_command(Command& cmd)
{
    if (websock.receive() < 0) { 
        if (websock.is_closed())
            return 0; // Connection closed by client
        else
            return -1;
    }

    if (websock.payload_size() < HEADER_LENGTH) {
        session_manager.kserver.syslog.print(SysLog::ERROR,
            "WebSocket: Command too small\n");
        return -1;
    }

    auto header_tuple
        = parse_buffer<HEADER_START, HEADER_TYPE_LIST>(websock.get_payload_no_copy());

    uint32_t payload_size = std::get<2>(header_tuple);

    if (payload_size > CMD_PAYLOAD_BUFFER_LEN) {
        DEBUG_MSG("WebSocket: Command payload buffer size too small\n");
        return -1;
    }

    if (payload_size + HEADER_LENGTH > websock.payload_size()) {
        session_manager.kserver.syslog.print(SysLog::ERROR,
            "WebSocket: Command payload reception incomplete\n");
        return -1;
    }

    if (payload_size + HEADER_LENGTH < websock.payload_size())
        session_manager.kserver.syslog.print(SysLog::WARNING,
            "WebSocket: Received more data than expected\n");

    bzero(cmd.buffer.data, cmd.buffer.size());
    memcpy(cmd.buffer.data, websock.get_payload_no_copy() + HEADER_LENGTH, payload_size);

    cmd.sess_id = id;
    cmd.device = static_cast<device_t>(std::get<0>(header_tuple));
    cmd.operation = std::get<1>(header_tuple);
    cmd.payload_size = payload_size;

    return HEADER_LENGTH + payload_size;
}

template<>
const uint32_t* Session<WEBSOCK>::RcvHandshake(uint32_t buff_size)
{
    // Handshaking
    if (Send<uint32_t>(buff_size) < 0) {
        session_manager.kserver.syslog.print(SysLog::ERROR,
            "WebSocket: Error sending the buffer size\n");
        return nullptr;
    }

    int payload_size = websock.receive();

    if (payload_size < 0)
        return nullptr;

    if (static_cast<uint32_t>(payload_size) != sizeof(uint32_t) * buff_size) {
        session_manager.kserver.syslog.print(SysLog::ERROR,
            "WebSocket: Invalid data size received\n");
        return nullptr;
    }

    if (websock.get_payload(recv_data_buff.data, recv_data_buff.size()) < 0)
        return nullptr;

    return reinterpret_cast<const uint32_t*>(recv_data_buff.data);
}

template<>
int Session<WEBSOCK>::SendCstr(const char *string)
{
    int err = websock.send(std::string(string));

    if (err < 0) {
        session_manager.kserver.syslog.print(SysLog::ERROR, 
                              "WebSocket::SendCstr: Can't write to client\n");
        return -1;
    }

    return strlen(string) + 1;
}
#endif

} // namespace kserver