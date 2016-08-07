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

    int SendCstr(const char *string);
    const uint32_t* RcvHandshake(uint32_t buff_size);
    template<typename... Tp> int Send(const std::tuple<Tp...>& t);
    template<typename T, size_t N> int Send(const std::array<T, N>& vect);
    template<typename T> int Send(const std::vector<T>& vect);
    template<typename T> int SendArray(const T* data, unsigned int len);
    template<class T> int Send(const T& data);

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

    int Run();

    inline unsigned int RequestNum(void) const {return requests_num;}
    inline unsigned int ErrorNum(void) const {return errors_num;}
    inline SessID GetID() const {return id;}
    inline const char* GetClientIP() const {return peer_info.ip_str;}
    inline int GetClientPort() const {return peer_info.port;}
    inline std::time_t GetStartTime() const {return start_time;}
    inline const SessionPermissions* GetPermissions() const {return &permissions;}

    // Receive - Send

    // TODO Move in Session<TCP> specialization
    int rcv_n_bytes(char *buffer, uint32_t n_bytes);

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
    const uint32_t* RcvHandshake(uint32_t buff_size);

    /// Send scalar data
    template<class T> int Send(const T& data);

    /// Send a C string
    /// @string The null-terminated string
    /// @return The number of bytes send if success, -1 if failure
    int SendCstr(const char* string);

    template<typename T> int SendArray(const T* data, unsigned int len);
    template<typename T> int Send(const std::vector<T>& vect);
    template<typename T, size_t N> int Send(const std::array<T, N>& vect);
    template<typename... Tp> int Send(const std::tuple<Tp...>& t);

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
        EmptyWebsock(std::shared_ptr<KServerConfig> config_, KServer *kserver_){}
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
int Session<sock_type>::Send(const std::vector<T>& vect)
{
    return SendArray<T>(vect.data(), vect.size());
}

template<int sock_type>
template<typename T, size_t N>
int Session<sock_type>::Send(const std::array<T, N>& vect)
{
    return SendArray<T>(vect.data(), N);
}

// http://stackoverflow.com/questions/1374468/stringstream-string-and-char-conversion-confusion
template<int sock_type>
template<typename... Tp>
int Session<sock_type>::Send(const std::tuple<Tp...>& t)
{
    const auto& arr = serialize(t);
    return SendArray<unsigned char>(arr.data(), arr.size());
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
        session_manager.kserver.syslog.print(SysLog::WARNING,
        "An error occured during session exit\n");
}

template<int sock_type>
int Session<sock_type>::Run()
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

#define SEND_SPECIALIZE_IMPL(session_kind)                              \
    template<> template<>                                               \
    inline int session_kind::Send<std::string>(const std::string& str)  \
    {                                                                   \
        return SendCstr(str.c_str());                                   \
    }                                                                   \
                                                                        \
    template<> template<>                                               \
    inline int session_kind::Send<uint32_t>(const uint32_t& val)        \
    {                                                                   \
        return SendArray<uint32_t>(&val, 1);                            \
    }                                                                   \
                                                                        \
    template<> template<>                                               \
    inline int session_kind::Send<uint64_t>(const uint64_t& val)        \
    {                                                                   \
        return SendArray<uint64_t>(&val, 1);                            \
    }                                                                   \
                                                                        \
    template<> template<>                                               \
    inline int session_kind::Send<float>(const float& val)              \
    {                                                                   \
        return SendArray<float>(&val, 1);                               \
    }                                                                   \
                                                                        \
    template<> template<>                                               \
    inline int session_kind::Send<double>(const double& val)            \
    {                                                                   \
        return SendArray<double>(&val, 1);                              \
    }

// -----------------------------------------------
// TCP
// -----------------------------------------------

#if KSERVER_HAS_TCP || KSERVER_HAS_UNIX_SOCKET

template<>
int Session<TCP>::rcv_n_bytes(char *buffer, uint32_t n_bytes);

template<> const uint32_t* Session<TCP>::RcvHandshake(uint32_t buff_size);
template<> int Session<TCP>::SendCstr(const char *string);

template<>
template<class T>
int Session<TCP>::SendArray(const T *data, unsigned int len)
{
    int bytes_send = sizeof(T) * len;
    int n_bytes_send = write(comm_fd, (void*)data, bytes_send);

    if (unlikely(n_bytes_send < 0)) {
       session_manager.kserver.syslog.print(SysLog::ERROR,
          "TCPSocket::SendArray: Can't write to client\n");
       return -1;
    }

    if (unlikely(n_bytes_send != bytes_send)) {
        session_manager.kserver.syslog.print(SysLog::ERROR,
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

template<> const uint32_t* Session<WEBSOCK>::RcvHandshake(uint32_t buff_size);

template<>
template<class T>
inline int Session<WEBSOCK>::SendArray(const T *data, unsigned int len)
{
    return websock.send(data, len);
}

template<>
inline int Session<WEBSOCK>::SendCstr(const char *string)
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
int SessionAbstract::Send(const T& data)
{
    SWITCH_SOCK_TYPE(template Send<T>(data))
    return -1;
}

template<typename T> 
int SessionAbstract::SendArray(const T* data, unsigned int len)
{
    SWITCH_SOCK_TYPE(template SendArray<T>(data, len))
    return -1;
}

template<typename T>
int SessionAbstract::Send(const std::vector<T>& vect)
{
    SWITCH_SOCK_TYPE(template Send<T>(vect));
    return -1;
}

template<typename T, size_t N>
int SessionAbstract::Send(const std::array<T, N>& vect)
{
    SWITCH_SOCK_TYPE(template Send<T, N>(vect))
    return -1;
}

template<typename... Tp>
int SessionAbstract::Send(const std::tuple<Tp...>& t)
{
    SWITCH_SOCK_TYPE(template Send<Tp...>(t))
    return -1;
}

inline const uint32_t* SessionAbstract::RcvHandshake(uint32_t buff_size)
{
    SWITCH_SOCK_TYPE(RcvHandshake(buff_size))
    return nullptr;
}

inline int SessionAbstract::SendCstr(const char *string)
{
    SWITCH_SOCK_TYPE(SendCstr(string))
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
