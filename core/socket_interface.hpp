/// Socket interface
///
/// (c) Koheron

#ifndef __SOCKET_INTERFACE_HPP__
#define __SOCKET_INTERFACE_HPP__

#include <string>
#include <vector>
#include <array>

#include "config.hpp"
#include "kserver.hpp"
#include "tuple_utils.hpp"

#if KSERVER_HAS_WEBSOCKET
#include "websocket.hpp"
#endif

#include <signal/kvector.hpp>

namespace kserver {

// TV, 16/08/2015
//
// Don't use virtual functions in the interface 
// because several send functions are templated
// and template functions cannot be virtual.
//
// As a consequence each function has to be
// declared in each derived class. The macro
// _SOCKET_INTERF_OBJ simplifies this task.
//
// This is a bit heavy, but not dramatic to maintain
// since I don't expect us to have a large amount of
// connection types.
//
// Probably something smarter can be done, but I'm
// not sure this will be clearer ...

/// Interface for socket calls
///
/// Provides an abstract interface for:
/// - Initialization/Exit of the socket
/// - Data reception
/// - Data sending
class SocketInterface
{
  public:
    // XXX TV 18/08/2015
    // Instead of passing all these parameters, I could directly
    // pass a pointer to the session 
    SocketInterface(KServerConfig *config_, KServer *kserver_, 
                    int comm_fd_, SessID id_)
    : config(config_), 
      kserver(kserver_),
      comm_fd(comm_fd_),
      id(id_)
    {
        bzero(recv_data_buff, KSERVER_RECV_DATA_BUFF_LEN);
    }
    
  protected:
    KServerConfig *config;
    KServer *kserver;
    int comm_fd;
    SessID id;
    
    char recv_data_buff[KSERVER_RECV_DATA_BUFF_LEN];  ///< Receive data buffer
}; // Socket

#define _SOCKET_INTERF_OBJ                                              \
  public:                                                               \
    int init(void);                                                     \
    int exit(void);                                                     \
                                                                        \
    int read_data(char *buff_str);                                      \
                                                                        \
    int RcvDataBuffer(uint32_t n_bytes);                                \
    const uint32_t* RcvHandshake(uint32_t buff_size);                   \
                                                                        \
    template<class T> int Send(const T& data);                          \
    template<typename T> int Send(const Klib::KVector<T>& vect);        \
    template<typename T> int Send(const std::vector<T>& vect);          \
                                                                        \
    template<typename T, std::size_t N>                                 \
        int Send(const std::array<T, N>& vect);                         \
                                                                        \
    int SendCstr(const char *string);                                   \
    template<class T> int SendArray(const T *data, unsigned int len);   \
    template<typename... Tp> int Send(const std::tuple<Tp...>& t);

#define SEND_KVECTOR(sock_interf)                       \
  template<typename T>                                  \
  int sock_interf::Send(const Klib::KVector<T>& vect)   \
  {                                                     \
      return SendArray<T>(vect.get_ptr(), vect.size()); \
  } 
  
#define SEND_STD_VECTOR(sock_interf)                    \
  template<typename T>                                  \
  int sock_interf::Send(const std::vector<T>& vect)     \
  {                                                     \
      return SendArray<T>(vect.data(), vect.size());    \
  }
  
#define SEND_STD_ARRAY(sock_interf)                     \
  template<typename T, size_t N>                        \
  int sock_interf::Send(const std::array<T, N>& vect)   \
  {                                                     \
      return SendArray<T>(vect.data(), N);              \
  }

// http://stackoverflow.com/questions/1374468/stringstream-string-and-char-conversion-confusion
#define SEND_TUPLE(sock_interf)                         \
  template<typename... Tp>                              \
  int sock_interf::Send(const std::tuple<Tp...>& t)     \
  {                                                     \
      std::stringstream ss;                             \
      stringify_tuple(t, ss);                           \
      ss << std::endl;                                  \
                                                        \
      const std::string& tmp = ss.str();                \
      return SendCstr(tmp.c_str());                     \
  }                                                     

// Functions whose implementation is the same for all derived class 
#define SEND_SPECIALIZE(sock_interf)                                     \
  template<> int sock_interf::Send<std::string>(const std::string& str); \
  template<> int sock_interf::Send<uint32_t>(const uint32_t& val);       \
  template<> int sock_interf::Send<uint64_t>(const uint64_t& val);       \
  template<> int sock_interf::Send<float>(const float& val); 

// -----------------------------------------------
// TCP
// -----------------------------------------------

#if KSERVER_HAS_TCP
class TCPSocketInterface : public SocketInterface
{
  _SOCKET_INTERF_OBJ

  public:
    TCPSocketInterface(KServerConfig *config_, KServer *kserver_, 
                       int comm_fd_, SessID id_)
    : SocketInterface(config_, kserver_, comm_fd_, id_)
    {
        bzero(read_str, KSERVER_READ_STR_LEN);
    }
    
  private:
    char read_str[KSERVER_READ_STR_LEN];  ///< Read string
}; // TCPSocketInterface

SEND_KVECTOR(TCPSocketInterface)
SEND_STD_VECTOR(TCPSocketInterface)
SEND_STD_ARRAY(TCPSocketInterface)
SEND_TUPLE(TCPSocketInterface)
SEND_SPECIALIZE(TCPSocketInterface)

template<class T>
int TCPSocketInterface::SendArray(const T *data, unsigned int len)
{
    int bytes_send = sizeof(T)*len;
    int n_bytes_send = write(comm_fd, (void*)data, bytes_send);
        
    if (n_bytes_send < 0) {
       kserver->syslog.print(SysLog::ERROR, 
                             "SendArray: Can't write to client\n");
       return -1;
    }
            
    if (n_bytes_send != bytes_send) {
        kserver->syslog.print(SysLog::ERROR, "Some bytes have not been sent\n");
        return -1;
    }

    return bytes_send;
}

#endif // KSERVER_HAS_TCP

// -----------------------------------------------
// Unix socket
// -----------------------------------------------

#if KSERVER_HAS_UNIX_SOCKET
// Unix socket has the same interface than TCP socket
typedef TCPSocketInterface UnixSocketInterface;
#endif // KSERVER_HAS_UNIX_SOCKET

// -----------------------------------------------
// WebSocket
// -----------------------------------------------

#if KSERVER_HAS_WEBSOCKET
class WebSocketInterface : public SocketInterface
{
  _SOCKET_INTERF_OBJ

  public:
    WebSocketInterface(KServerConfig *config_, KServer *kserver_, 
                       int comm_fd_, SessID id_)
    : SocketInterface(config_, kserver_, comm_fd_, id_),
      websock(config_, kserver_)
    {}
    
    ~WebSocketInterface() {}
      
  private:
    WebSocket websock;
}; // WebSocketInterface

SEND_KVECTOR(WebSocketInterface)
SEND_STD_VECTOR(WebSocketInterface)
SEND_STD_ARRAY(WebSocketInterface)
SEND_TUPLE(WebSocketInterface)
SEND_SPECIALIZE(WebSocketInterface)

template<class T>
int WebSocketInterface::SendArray(const T *data, unsigned int len)
{
    int bytes_send = websock.send<T>(data, len);
        
    if (bytes_send < 0) {
        kserver->syslog.print(SysLog::ERROR, 
                              "SendArray: Can't write to client\n");
        return -1;
    }

    return bytes_send;    
}

#endif // KSERVER_HAS_WEBSOCKET

} // namespace kserver

#endif // __SOCKET_INTERFACE_HPP__

