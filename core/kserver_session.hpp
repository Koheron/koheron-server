/// (c) Koheron

#ifndef __KSERVER_SESSION_HPP__
#define __KSERVER_SESSION_HPP__

#include <string>
#include <ctime>
#include <array>
#include <memory>

#include "commands.hpp"
#include "devices_manager.hpp"
#include "kserver_defs.hpp"
#include "kserver.hpp"
#include "peer_info.hpp"
#include "session_manager.hpp"
#include "socket_interface.hpp"

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
    Session(std::shared_ptr<KServerConfig> const& config_,
            int comm_fd, SessID id_, PeerInfo peer_info,
            SessionManager& session_manager_);

    /// Run the session
    int Run();

    /// Display the log of the session
    void DisplayLog(void);

    /// Number of requests received during the current session
    inline unsigned int RequestNum(void) const
    {
        return requests_num;
    }

    /// Number of requests errors during the current session
    inline unsigned int ErrorNum(void) const
    {
        return errors_num;
    }

    inline SessID GetID() const {return id;}
    inline const char* GetClientIP() const {return peer_info.ip_str;}
    inline int GetClientPort() const {return peer_info.port;}
    inline std::time_t GetStartTime() const {return start_time;}
    
    inline const SessionPermissions* GetPermissions() const
    {
        return &permissions;
    }

    // --- Receive

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

    /// Send Arrays
    template<typename T> int SendArray(const T* data, unsigned int len);
    template<typename T> int Send(const std::vector<T>& vect);
    template<typename T, size_t N> int Send(const std::array<T, N>& vect);

    /// Send a std::tuple
    template<typename... Tp> int Send(const std::tuple<Tp...>& t);

  private:
    std::shared_ptr<KServerConfig> config;
    int comm_fd;  ///< Socket file descriptor
    SessID id;
    SysLog *syslog_ptr;
    PeerInfo peer_info;
    SessionManager& session_manager;
    SessionPermissions permissions;

    // Monitoring
    unsigned int requests_num;
    unsigned int errors_num;
    std::time_t start_time;  ///< Starting time of the session

    std::unique_ptr<SocketInterface<sock_type>> socket;

    // -------------------
    // Internal functions
    int init_session();
    int exit_session();

    int read_command(Command& cmd);

friend class SessionManager;
};

template<int sock_type>
Session<sock_type>::Session(std::shared_ptr<KServerConfig> const& config_,
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
, requests_num(0)
, errors_num(0)
, start_time(0)
{
    socket = std::make_unique<SocketInterface<sock_type>>(
                config_, &session_manager.kserver, comm_fd, id_);
}

template<int sock_type>
int Session<sock_type>::init_session()
{
    errors_num = 0;
    requests_num = 0;
    start_time = std::time(nullptr);
    return socket->init();
}

template<int sock_type>
int Session<sock_type>::Run()
{
    if (init_session() < 0)
        return -1;

    while (!session_manager.kserver.exit_comm.load()) {    
        Command cmd;
        int nb_bytes_rcvd = socket->read_command(cmd);

        if (nb_bytes_rcvd <= 0) {
            exit_session();
            return nb_bytes_rcvd;
        }

        requests_num++;

        if (session_manager.dev_manager.Execute(cmd) < 0)
            errors_num++;
    }

    return 0;
}

template<int sock_type>
int Session<sock_type>::exit_session()
{
    return socket->exit();
}

#define SWITCH_SOCK_TYPE(...)                                       \
    switch (this->kind) {                                           \
      case TCP:                                                     \
        return static_cast<Session<TCP>*>(this)->__VA_ARGS__;       \
      case UNIX:                                                    \
        return static_cast<Session<UNIX>*>(this)->__VA_ARGS__;      \
      case WEBSOCK:                                                 \
        return static_cast<Session<WEBSOCK>*>(this)->__VA_ARGS__;   \
      default: assert(false);                                       \
    }

// For the template in the middle, see:
// http://stackoverflow.com/questions/1682844/templates-template-function-not-playing-well-with-classs-template-member-funct/1682885#1682885

template<int sock_type>
template<class T>
inline int Session<sock_type>::Send(const T& data)
{
    return socket->template Send<T>(data);
}

template<class T>
int SessionAbstract::Send(const T& data)
{
    SWITCH_SOCK_TYPE(template Send<T>(data))
    return -1;
}

template<int sock_type>
template<typename T> 
inline int Session<sock_type>::SendArray(const T* data, unsigned int len)
{
    return socket->template SendArray<T>(data, len);
}

template<typename T> 
int SessionAbstract::SendArray(const T* data, unsigned int len)
{
    SWITCH_SOCK_TYPE(template SendArray<T>(data, len))
    return -1;
}

template<int sock_type>
template<typename T>
inline int Session<sock_type>::Send(const std::vector<T>& vect)
{
    return socket->template Send<T>(vect);
}

template<typename T>
int SessionAbstract::Send(const std::vector<T>& vect)
{
    SWITCH_SOCK_TYPE(template Send<T>(vect));
    return -1;
}

template<int sock_type>
template<typename T, size_t N>
inline int Session<sock_type>::Send(const std::array<T, N>& vect)
{
    return socket->template Send<T, N>(vect);
}

template<typename T, size_t N>
int SessionAbstract::Send(const std::array<T, N>& vect)
{
    SWITCH_SOCK_TYPE(template Send<T, N>(vect))
    return -1;
}

template<int sock_type>
template<typename... Tp>
inline int Session<sock_type>::Send(const std::tuple<Tp...>& t)
{
    return socket->template Send<Tp...>(t);
}

template<typename... Tp>
int SessionAbstract::Send(const std::tuple<Tp...>& t)
{
    SWITCH_SOCK_TYPE(template Send<Tp...>(t))
    return -1;
}

template<int sock_type>
inline const uint32_t* Session<sock_type>::RcvHandshake(uint32_t buff_size)
{
    return socket->RcvHandshake(buff_size);
}

template<int sock_type>
inline int Session<sock_type>::SendCstr(const char *string)
{
    return socket->SendCstr(string);
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
