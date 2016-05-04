/// Implementation of kserver_session.hpp
///
/// (c) Koheron

#include "kserver_session.hpp"
#include "websocket.hpp"
#include "binary_parser.hpp"

namespace kserver {

Session::Session(KServerConfig *config_, int comm_fd_,
                 SessID id_, int sock_type_, PeerInfo peer_info_,
                 SessionManager& session_manager_)
: config(config_)
, comm_fd(comm_fd_)
, id(id_)
, syslog_ptr(&session_manager_.kserver.syslog)
, sock_type(sock_type_)
, peer_info(peer_info_)
, session_manager(session_manager_)
, permissions()
, requests_num(0)
, errors_num(0)
, start_time(0)
{
    assert(sock_type < sock_type_num);

    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP: {
        socket = new TCPSocketInterface(config_, &session_manager.kserver, 
                                        comm_fd, id_);
        break;
      }
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX: {
        socket = new UnixSocketInterface(config_, &session_manager.kserver, 
                                         comm_fd, id_);
        break;
      }
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK: {
        socket = new WebSocketInterface(config_, &session_manager.kserver, 
                                        comm_fd, id_);
        break;
      }
#endif
    }
}

Session::~Session()
{
    if(socket == nullptr)
        return;

    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP:
        delete TCPSOCKET;
        break;
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX:
        delete UNIXSOCKET;
        break;
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK:
        delete WEBSOCKET;
        break;
#endif
    }
}

int Session::init_session()
{
    // Initialize monitoring
    errors_num = 0;
    requests_num = 0;
    start_time = std::time(nullptr);
    
    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP:
        return TCPSOCKET->init();
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX:
        return UNIXSOCKET->init();
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK: 
        return WEBSOCKET->init();
#endif
    }
    
    return -1;
}

int Session::exit_session()
{
    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP:
        return TCPSOCKET->exit();
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX:
        return UNIXSOCKET->exit();
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK:
        return WEBSOCKET->exit();
#endif
    }
    
    return -1;
}

#define PROTOCOL_HEADER_LENGTH 12

int Session::read_command(Command& cmd)
{
    // Read header
    int header_bytes = rcv_n_bytes(PROTOCOL_HEADER_LENGTH);

    if (header_bytes <= 0)
        return header_bytes;

    // Decode header
    // |      RESERVED     | dev_id  |  op_id  |   payload_size    |   payload
    // |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | ...
    auto header = parse_buffer<4, uint16_t, uint16_t, uint32_t>(&buff_str[0]);
    uint16_t dev_id = std::get<0>(header);
    uint16_t op_id = std::get<1>(header);
    uint32_t payload_size = std::get<2>(header);

    int payload_bytes = rcv_n_bytes(payload_size);

    if (payload_bytes <= 0) 
        return payload_bytes;

    buff_str[payload_size] = '\0';

    printf("dev_id = %u\n", dev_id);
    printf("op_id = %u\n", op_id);
    printf("payload_size = %u\n", payload_size);
    printf("payload = %s\n", buff_str);

    cmd.sess_id = id;
    cmd.device = static_cast<device_t>(dev_id);
    cmd.operation = op_id;
    cmd.buffer = buff_str;

    return header_bytes + payload_bytes;
}

int Session::Run()
{
    if (init_session() < 0)
        return -1;
    
    while (!session_manager.kserver.exit_comm.load()) {    
        Command cmd;
        int nb_bytes_rcvd = read_command(cmd);

        if (nb_bytes_rcvd <= 0) {
            exit_session();
            return nb_bytes_rcvd;
        }

        if (session_manager.dev_manager.Execute(cmd) < 0)
            errors_num++;
    }

    return 0;
}

int Session::rcv_n_bytes(uint32_t n_bytes)
{
    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP:
        return TCPSOCKET->rcv_n_bytes(buff_str, n_bytes);
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX:
        return UNIXSOCKET->rcv_n_bytes(buff_str, n_bytes);
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK:
        return -1;
        // return WEBSOCKET->rcv_n_bytes(buff_str, n_bytes); // TODO
#endif
    }

    return -1;
}

const uint32_t* Session::RcvHandshake(uint32_t buff_size)
{
    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP:
        return TCPSOCKET->RcvHandshake(buff_size);
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX:
        return UNIXSOCKET->RcvHandshake(buff_size);
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK:
        return WEBSOCKET->RcvHandshake(buff_size);
#endif
    }
    
    return nullptr;
}

int Session::SendCstr(const char* string)
{
    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP:
        return TCPSOCKET->SendCstr(string);
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX:
        return UNIXSOCKET->SendCstr(string);
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK:
        return WEBSOCKET->SendCstr(string);
#endif
    }
    
    return -1;
}

} // namespace kserver
