/// (c) Koheron

#ifndef __KSERVER_SESSION_HPP__
#define __KSERVER_SESSION_HPP__

#include <string>
#include <ctime>
#include <vector>
#include <array>
#include <memory>
#include <unistd.h>
#include <type_traits>

#include "commands.hpp"
#include "devices_manager.hpp"
#include "kserver_defs.hpp"
#include "kserver.hpp"
#include "peer_info.hpp"
#include "session_manager.hpp"
#include "serializer_deserializer.hpp"
#include "socket_interface_defs.hpp"

#if KSERVER_HAS_WEBSOCKET
#include "websocket.hpp"
#include "websocket.tpp"
#endif

namespace kserver {

/// Stores the permissions of a session
struct SessionPermissions
{
    /// True if the session can write into a device
    bool write = DFLT_WRITE_PERM;
    /// True if the session can read from a device
    bool read = DFLT_READ_PERM;
};

class SessionManager;

class SessionAbstract
{
  public:
    SessionAbstract(int sock_type_)
    : kind(sock_type_) {}

    int send_cstr(const char *string);
    const uint32_t* rcv_handshake(uint32_t buff_size);
    template<typename T> int rcv_vector(std::vector<T>& vec, uint64_t length);
    template<typename... Tp> int send(const std::tuple<Tp...>& t);
    template<typename T, size_t N> int send(const std::array<T, N>& vect);
    template<typename T> int send(const std::vector<T>& vect);
    template<typename T> int send_array(const T* data, unsigned int len);
    template<class T> int send(const T& data);

    int kind;
};

/// Session
///
/// Receive and parse the client request for execution.
///
/// By calling the appropriate socket interface, it offers
/// an abstract communication layer to the devices. Thus
/// shielding them from the underlying communication protocol.
template<int sock_type>
class Session : public SessionAbstract
{
  public:
    Session(const std::shared_ptr<KServerConfig>& config_,
            int comm_fd, SessID id_, PeerInfo peer_info,
            SessionManager& session_manager_);

    int run();

    unsigned int request_num() const {return requests_num;}
    unsigned int error_num() const {return errors_num;}
    SessID get_id() const {return id;}
    const char* get_client_ip() const {return peer_info.ip_str;}
    int get_client_port() const {return peer_info.port;}
    std::time_t get_start_time() const {return start_time;}
    const SessionPermissions* get_permissions() const {return &permissions;}

    // Receive - Send

    // TODO Move in Session<TCP> specialization
    int rcv_n_bytes(char *buffer, uint64_t n_bytes);

    /// Receive data from client with handshaking
    /// @buff_size Size of the buffer to receive
    /// @return Pointer to the data if success, NULL else
    ///
    /// Handshaking protocol:
    /// 1) The size of the buffer must have been send as a 
    ///    command argument by the client
    /// 2) KServer acknowledges reception readiness by sending
    ///    the number of points to receive to the client
    /// 3) The client send the data buffer
    const uint32_t* rcv_handshake(uint32_t buff_size);

    template<typename T> int rcv_vector(std::vector<T>& vec, uint64_t length);

    /// Send scalar data
    template<class T> int send(const T& data);

    /// Send a C string
    /// @string The null-terminated string
    /// @return The number of bytes send if success, -1 if failure
    int send_cstr(const char* string);

    template<typename T> int send_array(const T* data, unsigned int len);
    template<typename T> int send(const std::vector<T>& vect);
    template<typename T, size_t N> int send(const std::array<T, N>& vect);
    template<typename... Tp> int send(const std::tuple<Tp...>& t);

  private:
    std::shared_ptr<KServerConfig> config;
    int comm_fd;  ///< Socket file descriptor
    SessID id;
    SysLog *syslog_ptr;
    PeerInfo peer_info;
    SessionManager& session_manager;
    SessionPermissions permissions;

    Command cmd;

    struct EmptyBuffer {};
    std::conditional_t<sock_type == TCP || sock_type == UNIX,
            Buffer<KSERVER_RECV_DATA_BUFF_LEN>, EmptyBuffer> recv_data_buff;

#if KSERVER_HAS_WEBSOCKET
    struct EmptyWebsock {
        EmptyWebsock(std::shared_ptr<KServerConfig> config_, KServer *kserver_) {}
    };

    std::conditional_t<sock_type == WEBSOCK, WebSocket, EmptyWebsock> websock;
#endif

    // Monitoring
    unsigned int requests_num; ///< Number of requests received during the current session
    unsigned int errors_num;   ///< Number of requests errors during the current session
    std::time_t start_time;    ///< Starting time of the session

    // Internal functions
    int init_socket();
    int exit_socket();
    int init_session();
    void exit_session();
    int read_command(Command& cmd);

friend class SessionManager;
};

template<int sock_type>
Session<sock_type>::Session(const std::shared_ptr<KServerConfig>& config_,
                            int comm_fd_, SessID id_, PeerInfo peer_info_,
                            SessionManager& session_manager_)
: SessionAbstract(sock_type)
, config(config_)
, comm_fd(comm_fd_)
, id(id_)
, syslog_ptr(&session_manager_.kserver.syslog)
, peer_info(peer_info_)
, session_manager(session_manager_)
, permissions()
, cmd()
#if KSERVER_HAS_WEBSOCKET
, websock(config_, &session_manager_.kserver)
#endif
, requests_num(0)
, errors_num(0)
, start_time(0)
{}

template<int sock_type>
template<typename T>
int Session<sock_type>::send(const std::vector<T>& vect)
{
    return send_array<T>(vect.data(), vect.size());
}

template<int sock_type>
template<typename T, size_t N>
int Session<sock_type>::send(const std::array<T, N>& vect)
{
    return send_array<T>(vect.data(), N);
}

// http://stackoverflow.com/questions/1374468/stringstream-string-and-char-conversion-confusion
template<int sock_type>
template<typename... Tp>
int Session<sock_type>::send(const std::tuple<Tp...>& t)
{
    const auto& arr = serialize(t);
    return send_array<unsigned char>(arr.data(), arr.size());
}

template<int sock_type>
int Session<sock_type>::init_session()
{
    errors_num = 0;
    requests_num = 0;
    start_time = std::time(nullptr);
    return init_socket();
}

template<int sock_type>
void Session<sock_type>::exit_session()
{
    if (exit_socket() < 0)
        session_manager.kserver.syslog.print<SysLog::WARNING>(
        "An error occured during session exit\n");
}

template<int sock_type>
int Session<sock_type>::run()
{
    if (init_session() < 0)
        return -1;

    while (!session_manager.kserver.exit_comm.load()) {
        int nb_bytes_rcvd = read_command(cmd);

        if (session_manager.kserver.exit_comm.load())
            break;

        if (nb_bytes_rcvd <= 0) {

            // We don't call exit_session() here because the
            // socket is already closed.

            return nb_bytes_rcvd;
        }

        requests_num++;

        if (unlikely(session_manager.dev_manager.Execute(cmd) < 0))
            errors_num++;
    }

    exit_session();
    return 0;
}

#define SEND_SPECIALIZE_IMPL(session_kind)                                            \
    template<> template<>                                                             \
    inline int session_kind::send<std::string>(const std::string& str)                \
    {                                                                                 \
        return send_cstr(str.c_str());                                                \
    }                                                                                 \
                                                                                      \
    template<> template<>                                                             \
    inline int session_kind::send<uint32_t>(const uint32_t& val)                      \
    {                                                                                 \
        return send_array<uint32_t>(&val, 1);                                         \
    }                                                                                 \
                                                                                      \
    template<> template<>                                                             \
    inline int session_kind::send<bool>(const bool& val)                              \
    {                                                                                 \
        return send<uint32_t>(val);                                                   \
    }                                                                                 \
                                                                                      \
    template<> template<>                                                             \
    inline int session_kind::send<int>(const int& val)                                \
    {                                                                                 \
        return send<uint32_t>(val);                                                   \
    }                                                                                 \
                                                                                      \
    template<> template<>                                                             \
    inline int session_kind::send<uint64_t>(const uint64_t& val)                      \
    {                                                                                 \
        return send_array<uint64_t>(&val, 1);                                         \
    }                                                                                 \
                                                                                      \
    template<> template<>                                                             \
    inline int session_kind::send<float>(const float& val)                            \
    {                                                                                 \
        return send_array<float>(&val, 1);                                            \
    }                                                                                 \
                                                                                      \
    template<> template<>                                                             \
    inline int session_kind::send<double>(const double& val)                          \
    {                                                                                 \
        return send_array<double>(&val, 1);                                           \
    }

// -----------------------------------------------
// TCP
// -----------------------------------------------

#if KSERVER_HAS_TCP || KSERVER_HAS_UNIX_SOCKET

template<>
int Session<TCP>::rcv_n_bytes(char *buffer, uint64_t n_bytes);

template<> const uint32_t* Session<TCP>::rcv_handshake(uint32_t buff_size);
template<> int Session<TCP>::send_cstr(const char *string);

template<>
template<typename T>
int Session<TCP>::rcv_vector(std::vector<T>& vec, uint64_t length)
{
    vec.resize(length);
    return rcv_n_bytes(reinterpret_cast<char *>(vec.data()), length * sizeof(T));
}

template<>
template<class T>
int Session<TCP>::send_array(const T *data, unsigned int len)
{
    int bytes_send = sizeof(T) * len;
    int n_bytes_send = write(comm_fd, (void*)data, bytes_send);

    if (unlikely(n_bytes_send < 0)) {
       session_manager.kserver.syslog.print<SysLog::ERROR>(
          "TCPSocket::SendArray: Can't write to client\n");
       return -1;
    }

    if (unlikely(n_bytes_send != bytes_send)) {
        session_manager.kserver.syslog.print<SysLog::ERROR>(
            "TCPSocket::SendArray: Some bytes have not been sent\n");
        return -1;
    }

    if (config->verbose)
        session_manager.kserver.syslog.print_dbg("[S] [%u bytes]\n", bytes_send);

    return bytes_send;
}

SEND_SPECIALIZE_IMPL(Session<TCP>)

#endif // KSERVER_HAS_TCP

// -----------------------------------------------
// Unix socket
// -----------------------------------------------

#if KSERVER_HAS_UNIX_SOCKET
// Unix socket has the same interface than TCP socket
template<>
class Session<UNIX> : public Session<TCP>
{
  public:
    Session<UNIX>(const std::shared_ptr<KServerConfig>& config_,
                  int comm_fd_, SessID id_, PeerInfo peer_info_,
                  SessionManager& session_manager_)
    : Session<TCP>(config_, comm_fd_, id_, peer_info_, session_manager_) {}
};
#endif // KSERVER_HAS_UNIX_SOCKET

// -----------------------------------------------
// WebSocket
// -----------------------------------------------

#if KSERVER_HAS_WEBSOCKET

template<> const uint32_t* Session<WEBSOCK>::rcv_handshake(uint32_t buff_size);

template<>
template<typename T>
int Session<WEBSOCK>::rcv_vector(std::vector<T>& vec, uint64_t length)
{
    // TODO
    return -1;
}

template<>
template<class T>
inline int Session<WEBSOCK>::send_array(const T *data, unsigned int len)
{
    return websock.send(data, len);
}

template<>
inline int Session<WEBSOCK>::send_cstr(const char *string)
{
    return websock.send_cstr(string);
}

SEND_SPECIALIZE_IMPL(Session<WEBSOCK>)

#endif // KSERVER_HAS_WEBSOCKET

// -----------------------------------------------
// Select session kind
// -----------------------------------------------

#if KSERVER_HAS_TCP
  #define CASE_TCP(...)                                             \
    case TCP:                                                       \
        return static_cast<Session<TCP>*>(this)-> __VA_ARGS__;
#else
  #define CASE_TCP(...)
#endif

#if KSERVER_HAS_UNIX_SOCKET
  #define CASE_UNIX(...)                                            \
    case UNIX:                                                      \
        return static_cast<Session<UNIX>*>(this)-> __VA_ARGS__;
#else
  #define CASE_UNIX(...)
#endif

#if KSERVER_HAS_WEBSOCKET
  #define CASE_WEBSOCK(...)                                         \
    case WEBSOCK:                                                   \
        return static_cast<Session<WEBSOCK>*>(this)-> __VA_ARGS__;
#else
  #define CASE_WEBSOCK(...)
#endif

#define SWITCH_SOCK_TYPE(...)     \
    switch (this->kind) {         \
      CASE_TCP(__VA_ARGS__)       \
      CASE_UNIX(__VA_ARGS__)      \
      CASE_WEBSOCK(__VA_ARGS__)   \
      default: assert(false);     \
    }

// For the template in the middle, see:
// http://stackoverflow.com/questions/1682844/templates-template-function-not-playing-well-with-classs-template-member-funct/1682885#1682885

template<class T>
int SessionAbstract::send(const T& data)
{
    SWITCH_SOCK_TYPE(template send<T>(data))
    return -1;
}

template<typename T> 
int SessionAbstract::send_array(const T* data, unsigned int len)
{
    SWITCH_SOCK_TYPE(template send_array<T>(data, len))
    return -1;
}

template<typename T>
int SessionAbstract::send(const std::vector<T>& vect)
{
    SWITCH_SOCK_TYPE(template send<T>(vect));
    return -1;
}

template<typename T, size_t N>
int SessionAbstract::send(const std::array<T, N>& vect)
{
    SWITCH_SOCK_TYPE(template send<T, N>(vect))
    return -1;
}

template<typename... Tp>
int SessionAbstract::send(const std::tuple<Tp...>& t)
{
    SWITCH_SOCK_TYPE(template send<Tp...>(t))
    return -1;
}

template<typename T>
inline int SessionAbstract::rcv_vector(std::vector<T>& vec, uint64_t length)
{
    SWITCH_SOCK_TYPE(rcv_vector(vec, length))
    return -1;
}

inline const uint32_t* SessionAbstract::rcv_handshake(uint32_t buff_size)
{
    SWITCH_SOCK_TYPE(rcv_handshake(buff_size))
    return nullptr;
}

inline int SessionAbstract::send_cstr(const char *string)
{
    SWITCH_SOCK_TYPE(send_cstr(string))
    return -1;
}

// Cast abstract session unique_ptr
template<int sock_type>
Session<sock_type>*
cast_to_session(const std::unique_ptr<SessionAbstract>& sess_abstract)
{
    return static_cast<Session<sock_type>*>(sess_abstract.get());
}

} // namespace kserver

#endif // __KSERVER_SESSION_HPP__
