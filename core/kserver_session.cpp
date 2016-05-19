/// Implementation of kserver_session.hpp
///
/// (c) Koheron

#include "kserver_session.hpp"
#include "websocket.hpp"

namespace kserver {

Session::Session(KServerConfig *config_, int comm_fd_,
                 SessID id_, PeerInfo peer_info_,
                 SessionManager& session_manager_)
: config(config_)
, comm_fd(comm_fd_)
, id(id_)
, syslog_ptr(&session_manager_.kserver.syslog)
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

#if KSERVER_HAS_TCP
template<>
int Session<TCP>::read_command(Command& cmd)
{
    return TCPSOCKET->read_command(cmd);
}
#endif

#if KSERVER_HAS_UNIX_SOCKET
template<>
int Session<UNIX>::read_command(Command& cmd)
{
    return UNIXSOCKET->read_command(cmd);
}
#endif

#if KSERVER_HAS_WEBSOCKET
template<>
int Session<WEBSOCK>::read_command(Command& cmd)
{
    return WEBSOCKET->read_command(cmd);
}
#endif

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
