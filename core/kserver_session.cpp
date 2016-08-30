/// (c) Koheron

#include "kserver_session.hpp"

namespace kserver {

// -----------------------------------------------
// TCP
// -----------------------------------------------

#if KSERVER_HAS_TCP || KSERVER_HAS_UNIX_SOCKET

template<> int Session<TCP>::init_socket() {return 0;}
template<> int Session<TCP>::exit_socket() {return 0;}

#define HEADER_TYPE_LIST uint16_t, uint16_t, uint32_t

template<>
int Session<TCP>::read_command(Command& cmd)
{
    // Read and decode header
    // |      RESERVED     | dev_id  |  op_id  |   payload_size    |   payload
    // |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | ...
    int header_bytes = rcv_n_bytes(cmd.header.data(), Command::HEADER_SIZE);

    if (header_bytes == 0)
        return header_bytes;

    if (unlikely(header_bytes < 0)) {
        session_manager.kserver.syslog.print<SysLog::ERROR>(
            "TCPSocket: Cannot read header\n");
        return header_bytes;
    }

    auto header_tuple = cmd.header.deserialize<HEADER_TYPE_LIST>();
    uint32_t payload_size = std::get<2>(header_tuple);

    cmd.sess_id = id;
    cmd.device = static_cast<device_t>(std::get<0>(header_tuple));
    cmd.operation = std::get<1>(header_tuple);
    cmd.payload_size = payload_size;

    if (payload_size > cmd.payload.size()) {
        session_manager.kserver.syslog.print<SysLog::ERROR>(
                  "TCPSocket: Command payload buffer size too small\n"
                  "payload_size = %u cmd.payload.size = %u\n",
                  payload_size, cmd.payload.size());
        return -1;
    }

    int payload_bytes = 0;

    if (payload_size > 0) {
        payload_bytes = rcv_n_bytes(cmd.payload.data(), payload_size);

        if (payload_bytes == 0)
            return payload_bytes;

        if (unlikely(payload_bytes < 0)) {
            session_manager.kserver.syslog.print<SysLog::ERROR>(
                "TCPSocket: Cannot read payload for device %u, operation %u\n",
                cmd.device, cmd.operation);
            return header_bytes;
        }
    }

    if (config->verbose)
        session_manager.kserver.syslog.print_dbg(
            "TCPSocket: Receive command for device %u, operation %u [%u bytes]\n",
            cmd.device, cmd.operation, cmd.payload_size);

    return header_bytes + payload_bytes;
}

// TODO Replace by function load_buffer
template<>
int Session<TCP>::rcv_n_bytes(char *buffer, uint64_t n_bytes)
{
    int bytes_rcv = 0;
    uint32_t bytes_read = 0;

    while (bytes_read < n_bytes) {
        bytes_rcv = read(comm_fd, buffer + bytes_read, n_bytes - bytes_read);

        if (bytes_rcv == 0) {
            session_manager.kserver.syslog.print<SysLog::INFO>(
                "TCPSocket: Connection closed by client\n");
            return 0;
        }

        if (unlikely(bytes_rcv < 0)) {
            session_manager.kserver.syslog.print<SysLog::ERROR>(
                "TCPSocket: Can't receive data\n");
            return -1;
        }

        bytes_read += bytes_rcv;
    }

    assert(bytes_read == n_bytes);

    if (config->verbose)
        session_manager.kserver.syslog.print_dbg("[R@%u] [%u bytes]\n",
                                                 id, bytes_read);

    return bytes_read;
}

template<>
const uint32_t* Session<TCP>::rcv_handshake(uint32_t buff_size)
{
    if (unlikely(send<uint32_t>(htonl(buff_size)) < 0)) {
        session_manager.kserver.syslog.print<SysLog::ERROR>(
            "TCPSocket: Cannot send buffer size\n");
        return nullptr;
    }

    int n_bytes_received = rcv_n_bytes(recv_data_buff.data(), sizeof(uint32_t) * buff_size);

    if (unlikely(n_bytes_received < 0))
        return nullptr;

    if (config->verbose)
        session_manager.kserver.syslog.print_dbg("[R@%u] [%u bytes]\n",
                                                 id, n_bytes_received);

    return reinterpret_cast<const uint32_t*>(recv_data_buff.data());
}

#endif

// -----------------------------------------------
// WebSocket
// -----------------------------------------------

#if KSERVER_HAS_WEBSOCKET

template<>
int Session<WEBSOCK>::init_socket()
{
    websock.set_id(comm_fd);

    if (websock.authenticate() < 0) {
        session_manager.kserver.syslog.print<SysLog::CRITICAL>(
                              "Cannot connect websocket to client\n");
        return -1;
    }

    return 0;
}

template<> int Session<WEBSOCK>::exit_socket()
{
    return websock.exit();
}

template<>
int Session<WEBSOCK>::read_command(Command& cmd)
{
    if (unlikely(websock.receive_cmd(cmd) < 0))
        return -1;

    if (websock.is_closed())
        return 0;

    if (websock.payload_size() < Command::HEADER_SIZE) {
        session_manager.kserver.syslog.print<SysLog::ERROR>(
            "WebSocket: Command too small\n");
        return -1;
    }

    auto header_tuple = cmd.header.deserialize<HEADER_TYPE_LIST>();
    uint32_t payload_size = std::get<2>(header_tuple);

    if (unlikely(payload_size > CMD_PAYLOAD_BUFFER_LEN)) {
        DEBUG_MSG("WebSocket: Command payload buffer size too small\n");
        return -1;
    }

    if (payload_size + Command::HEADER_SIZE > websock.payload_size()) {
        session_manager.kserver.syslog.print<SysLog::ERROR>(
            "WebSocket: Command payload reception incomplete. "
            "Expected %zu bytes. Received %zu bytes.\n",
            payload_size + Command::HEADER_SIZE, websock.payload_size());
        return -1;
    }

    // if (payload_size + Command::HEADER_SIZE < websock.payload_size())
    //     session_manager.kserver.syslog.print<SysLog::WARNING>(
    //         "WebSocket: Received more data than expected. "
    //         "Expected %zu bytes. Received %zu bytes.\n",
    //         payload_size + Command::HEADER_SIZE, websock.payload_size());

    cmd.sess_id = id;
    cmd.device = static_cast<device_t>(std::get<0>(header_tuple));
    cmd.operation = std::get<1>(header_tuple);
    cmd.payload_size = payload_size;

    return Command::HEADER_SIZE + payload_size;
}

template<>
const uint32_t* Session<WEBSOCK>::rcv_handshake(uint32_t buff_size)
{
    if (unlikely(send<uint32_t>(buff_size) < 0)) {
        session_manager.kserver.syslog.print<SysLog::ERROR>(
            "WebSocket: Error sending the buffer size\n");
        return nullptr;
    }

    int payload_size = websock.receive();

    if (unlikely(payload_size < 0))
        return nullptr;

    if (unlikely(static_cast<uint32_t>(payload_size) != sizeof(uint32_t) * buff_size)) {
        session_manager.kserver.syslog.print<SysLog::ERROR>(
            "WebSocket: Invalid data size received\n");
        return nullptr;
    }

    return reinterpret_cast<const uint32_t*>(websock.get_payload_no_copy());
}
#endif

} // namespace kserver
