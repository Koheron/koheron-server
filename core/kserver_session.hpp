/// (c) Koheron

#ifndef __KSERVER_SESSION_HPP__
#define __KSERVER_SESSION_HPP__

#include <string>
#include <ctime>
#include <array>

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

    inline int GetSockType() const {return kind;}

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
    Session(KServerConfig *config_, int comm_fd,
            SessID id_, PeerInfo peer_info,
            SessionManager& session_manager_);

    ~Session();

    /// Run the session
    int Run();

    // --- Accessors

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

    inline SessID GetID() const               { return id;               }
    inline const char* GetClientIP() const    { return peer_info.ip_str; }
    inline int GetClientPort() const          { return peer_info.port;   }
    inline std::time_t GetStartTime() const   { return start_time;       }
    
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
    KServerConfig *config;
    int comm_fd;                ///< Socket file descriptor
    SessID id;                  ///< Session ID
    SysLog *syslog_ptr;
    PeerInfo peer_info;
    SessionManager& session_manager;
    SessionPermissions permissions;

    // -------------------
    // Monitoring
    unsigned int requests_num;
    unsigned int errors_num;

    std::time_t start_time;     ///< Starting time od the session
    // -------------------

    SocketInterface<sock_type> *socket;

    // -------------------
    // Internal functions
    int init_session();
    int exit_session();

    int read_command(Command& cmd);

friend class SessionManager;
};

template<int sock_type>
Session<sock_type>::Session(KServerConfig *config_, int comm_fd_,
                            SessID id_, PeerInfo peer_info_,
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
    socket = new SocketInterface<sock_type>(config_, &session_manager.kserver,
                                            comm_fd, id_);
}

template<int sock_type>
Session<sock_type>::~Session()
{
    if (socket != nullptr)
        delete socket;
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

// For the template in the middle, see:
// http://stackoverflow.com/questions/1682844/templates-template-function-not-playing-well-with-classs-template-member-funct/1682885#1682885

template<int sock_type>
template<class T>
int Session<sock_type>::Send(const T& data)
{
    return socket->template Send<T>(data);
}

template<int sock_type>
template<typename T> 
int Session<sock_type>::SendArray(const T* data, unsigned int len)
{
    return socket->template SendArray<T>(data, len);
}

template<int sock_type>
template<typename T>
int Session<sock_type>::Send(const std::vector<T>& vect)
{
    return socket->template Send<T>(vect);
}

template<int sock_type>
template<typename T, size_t N>
int Session<sock_type>::Send(const std::array<T, N>& vect)
{
    return socket->template Send<T, N>(vect);
}

template<int sock_type>
template<typename... Tp>
int Session<sock_type>::Send(const std::tuple<Tp...>& t)
{
    return socket->template Send<Tp...>(t);
}

template<int sock_type>
const uint32_t* Session<sock_type>::RcvHandshake(uint32_t buff_size)
{
    return socket->RcvHandshake(buff_size);
}

template<int sock_type>
int Session<sock_type>::SendCstr(const char *string)
{
    return socket->SendCstr(string);
}

#define TCP_SESSION  static_cast<Session<TCP>*>(&session)
#define UNIX_SESSION static_cast<Session<UNIX>*>(&session)
#define WEB_SESSION  static_cast<Session<WEBSOCK>*>(&session)

int SendCstr(SessionAbstract& session, const char *string)
{
    switch(session.kind) {
        case TCP:
            return TCP_SESSION->SendCstr(string);
            break;
        case UNIX:
            return UNIX_SESSION->SendCstr(string);
            break;
        case WEBSOCK:
            return WEB_SESSION->SendCstr(string);
            break;
    }

    return -1;
}

} // namespace kserver

#endif // __KSERVER_SESSION_HPP__
