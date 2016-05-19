///
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
#include "socket_interface.hpp"
#include "peer_info.hpp"

namespace kserver {

/// Stores the permissions of a session
struct SessionPermissions
{
    // XXX TV 27/09/2015
    //Should these values be atomic ?

    /// True if the session can write into a device
    bool write = DFLT_WRITE_PERM;
    
    /// True if the session can read from a device
    bool read = DFLT_READ_PERM;
};

class SessionManager;

/// Session
///
/// Receive and parse the client request for execution.
///
/// By calling the appropriate socket interface, it offers
/// an abstract communication layer to the devices. Thus
/// shielding them from the underlying communication protocol.
template<int sock_type>
class Session
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
    inline int GetSockType() const            { return sock_type;        }
    inline const char* GetClientIP() const    { return peer_info.ip_str; }
    inline int GetClientPort() const          { return peer_info.port;   }
    inline std::time_t GetStartTime() const   { return start_time;       }
    
    inline const SessionPermissions* GetPermissions() const
    {
        return &permissions;
    }
    
    // --- Receive 
    // For large amount of data transfer
    
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
    
    SocketInterface *socket;
    
    // -------------------
    // Internal functions
    int init_session();
    int exit_session();

    template<int sock_type>
    int read_command(Command& cmd);
    
friend class SessionManager;
};

template<int <sock_type>
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

        requests_num++;

        if (session_manager.dev_manager.Execute(cmd) < 0)
            errors_num++;
    }

    return 0;
}

#define TCPSOCKET  static_cast<TCPSocketInterface*>(socket)
#define UNIXSOCKET static_cast<UnixSocketInterface*>(socket)
#define WEBSOCKET  static_cast<WebSocketInterface*>(socket)

// For the template in the middle, see:
// http://stackoverflow.com/questions/1682844/templates-template-function-not-playing-well-with-classs-template-member-funct/1682885#1682885

#if KSERVER_HAS_TCP
template<>
template<class T>
int Session<TCP>::Send(const T& data)
{
    return TCPSOCKET->template Send<T>(data);
}

template<>
template<typename T> 
int Session<TCP>::SendArray(const T* data, unsigned int len)
{
    return TCPSOCKET->template SendArray<T>(data, len);
}

template<>
template<typename T>
int Session<TCP>::Send(const std::vector<T>& vect)
{
    return TCPSOCKET->template Send<T>(vect);
}

template<>
template<typename T, size_t N>
int Session<TCP>::Send(const std::array<T, N>& vect)
{
    return TCPSOCKET->template Send<T, N>(vect);
}

template<>
template<typename... Tp>
int Session<TCP>::Send(const std::tuple<Tp...>& t)
{
    return TCPSOCKET->template Send<Tp...>(t);
}
#endif // KSERVER_HAS_TCP

#if KSERVER_HAS_UNIX_SOCKET
template<>
template<class T>
int Session<UNIX>::Send(const T& data)
{
    return UNIXSOCKET->template Send<T>(data);
}

template<>
template<typename T> 
int Session<UNIX>::SendArray(const T* data, unsigned int len)
{
    return UNIXSOCKET->template SendArray<T>(data, len);
}

template<>
template<typename T>
int Session<UNIX>::Send(const std::vector<T>& vect)
{
    return UNIXSOCKET->template Send<T>(vect);
}

template<>
template<typename T, size_t N>
int Session<UNIX>::Send(const std::array<T, N>& vect)
{
    return UNIXSOCKET->template Send<T, N>(vect);
}

template<>
template<typename... Tp>
int Session<UNIX>::Send(const std::tuple<Tp...>& t)
{
    return UNIXSOCKET->template Send<Tp...>(t);
}
#endif // KSERVER_HAS_UNIX_SOCKET

#if KSERVER_HAS_WEBSOCKET
template<>
template<class T>
int Session<WEBSOCK>::Send(const T& data)
{
    return WEBSOCKET->template Send<T>(data);
}

template<>
template<typename T> 
int Session<WEBSOCK>::SendArray(const T* data, unsigned int len)
{
    return WEBSOCKET->template SendArray<T>(data, len);
}

template<>
template<typename T>
int Session<WEBSOCK>::Send(const std::vector<T>& vect)
{
    return WEBSOCKET->template Send<T>(vect);
}

template<>
template<typename T, size_t N>
int Session<WEBSOCK>::Send(const std::array<T, N>& vect)
{
    return WEBSOCKET->template Send<T, N>(vect);
}

template<>
template<typename... Tp>
int Session<WEBSOCK>::Send(const std::tuple<Tp...>& t)
{
    return WEBSOCKET->template Send<Tp...>(t);
}
#endif // KSERVER_HAS_WEBSOCKET

} // namespace kserver

#endif // __KSERVER_SESSION_HPP__

