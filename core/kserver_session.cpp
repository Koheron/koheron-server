/// Implementation of kserver_session.hpp
///
/// (c) Koheron

#include "kserver_session.hpp"
#include "websocket.hpp"

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

int Session::init_session(void)
{    
    cmd_list = std::vector<Command>(0);

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

int Session::exit_session(void)
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
    int header_bytes = TCPSOCKET->rcv_n_bytes(buff_str, PROTOCOL_HEADER_LENGTH);

    if (header_bytes <= 0)
        return header_bytes;

    // Decode header
    // |      RESERVED     | dev_id  |  op_id  |   payload_size    |   payload
    // |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | ...
    uint16_t dev_id = buff_str[5] + (buff_str[4] << 8);
    uint16_t op_id = buff_str[7] + (buff_str[6] << 8);
    uint32_t payload_size = buff_str[11] + (buff_str[10] << 8) 
                            + (buff_str[9] << 16) + (buff_str[8] << 24);

    int payload_bytes = TCPSOCKET->rcv_n_bytes(buff_str, payload_size);

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

void Session::execute_cmd(Command& cmd)
{    
    int exec_status = session_manager.dev_manager.Execute(cmd);
    
    if (exec_status < 0)
        errors_num++;
}

void Session::execute_cmds()
{
    for (unsigned int i=0; i<cmd_list.size(); i++)
        execute_cmd(cmd_list[i]);
}

int Session::Run()
{
    if (init_session() < 0)
        return -1;
    
    while (!session_manager.kserver.exit_comm.load()) {    
        // Read
        int nb_bytes_rcvd = -1;
        
        switch (sock_type) {
#if KSERVER_HAS_TCP
          case TCP: {
            Command cmd;
            nb_bytes_rcvd = read_command(cmd);

            if (nb_bytes_rcvd == 0) {
                goto exit;
            } else if (nb_bytes_rcvd < 0) {
                exit_session();
                return nb_bytes_rcvd;
            }

            execute_cmd(cmd);

            break;
          }
#endif
#if KSERVER_HAS_UNIX_SOCKET
          case UNIX: {
            nb_bytes_rcvd = UNIXSOCKET->read_data(buff_str);
            break;
          }
#endif
#if KSERVER_HAS_WEBSOCKET
          case WEBSOCK: {
            nb_bytes_rcvd = WEBSOCKET->read_data(buff_str);

            if (nb_bytes_rcvd == 0) {
                goto exit;
            } else if (nb_bytes_rcvd < 0) {
                exit_session();
                return nb_bytes_rcvd;
            }
     
            // Parse and execute
            if (parse_input_buffer(nb_bytes_rcvd) == 1) // Request not complete
                continue;
            else
                execute_cmds();

            break;
          }
#endif
        }
    }

exit:
    exit_session(); 
    return 0;
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

