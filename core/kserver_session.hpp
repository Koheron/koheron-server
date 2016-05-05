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
#include "binary_parser.hpp"

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
class Session
{
  public:
    Session(KServerConfig *config_, int comm_fd,
            SessID id_, int sock_type, PeerInfo peer_info,
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
    int rcv_n_bytes(uint32_t n_bytes);

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
    
    template<typename T> int Send(const Klib::KVector<T>& vect);
    template<typename T> int Send(const std::vector<T>& vect);
    template<typename T, size_t N> int Send(const std::array<T, N>& vect);
    
    /// Send a std::tuple
    template<typename... Tp> int Send(const std::tuple<Tp...>& t);
    
    // --- Internal use
    int init(void);
    int exit(void);
    int read_data(char *buff_str, char *remain_str);
    
  private:
    KServerConfig *config;
    int comm_fd;                ///< Socket file descriptor
    SessID id;                  ///< Session ID
    SysLog *syslog_ptr;
    int sock_type;              ///< Type of socket (TCP or websocket)
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
    
    // Reception buffer
    char buff_str[2*KSERVER_READ_STR_LEN];
    
    // -------------------
    // Internal functions
    int init_session();
    int exit_session();

    int read_command(Command& cmd);
    
friend class SessionManager;
};

#define TCPSOCKET  static_cast<TCPSocketInterface*>(socket)
#define UNIXSOCKET static_cast<UnixSocketInterface*>(socket)
#define WEBSOCKET  static_cast<WebSocketInterface*>(socket)

// For the template in the middle, see:
// http://stackoverflow.com/questions/1682844/templates-template-function-not-playing-well-with-classs-template-member-funct/1682885#1682885
template<class T> 
int Session::Send(const T& data)
{
    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP:
        return TCPSOCKET->template Send<T>(data);
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX:
        return UNIXSOCKET->template Send<T>(data);
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK:
        return WEBSOCKET->template Send<T>(data);
#endif
    }
    
    return -1;
}

template<typename T> 
int Session::SendArray(const T* data, unsigned int len)
{
    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP:
        return TCPSOCKET->template SendArray<T>(data, len);
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX:
        return UNIXSOCKET->template SendArray<T>(data, len);
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK:
        return WEBSOCKET->template SendArray<T>(data, len);
#endif
    }
    
    return -1;
}

template<typename T>
int Session::Send(const Klib::KVector<T>& vect)
{
    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP:
        return TCPSOCKET->template Send<T>(vect);
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX:
        return UNIXSOCKET->template Send<T>(vect);
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK:
        return WEBSOCKET->template Send<T>(vect);
#endif
    }
    
    return -1;
}

template<typename T>
int Session::Send(const std::vector<T>& vect)
{
    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP:
        return TCPSOCKET->template Send<T>(vect);
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX:
        return UNIXSOCKET->template Send<T>(vect);
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK:
        return WEBSOCKET->template Send<T>(vect);
#endif
    }
    
    return -1;
}

template<typename T, size_t N>
int Session::Send(const std::array<T, N>& vect)
{
    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP:
        return TCPSOCKET->template Send<T, N>(vect);
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX:
        return UNIXSOCKET->template Send<T, N>(vect);
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK:
        return WEBSOCKET->template Send<T, N>(vect);
#endif
    }
    
    return -1;
}

template<typename... Tp>
int Session::Send(const std::tuple<Tp...>& t)
{
    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP:
        return TCPSOCKET->template Send<Tp...>(t);
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX:
        return UNIXSOCKET->template Send<Tp...>(t);
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK:
        return WEBSOCKET->template Send<Tp...>(t);
#endif
    }
    
    return -1;
}

} // namespace kserver

#endif // __KSERVER_SESSION_HPP__

