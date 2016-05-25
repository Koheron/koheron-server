/// Socket interface
///
/// (c) Koheron

#ifndef __SOCKET_INTERFACE_HPP__
#define __SOCKET_INTERFACE_HPP__

#include <string>
#include <vector>
#include <array>

#include "socket_interface_defs.hpp"
#include "config.hpp"
#include "kserver.hpp"
#include "tuple_utils.hpp"
#include "commands.hpp"
#include "binary_parser.hpp"

#if KSERVER_HAS_WEBSOCKET
#include "websocket.hpp"
#endif

namespace kserver {

/// Interface for socket calls
///
/// Provides an abstract interface for:
/// - Initialization/Exit of the socket
/// - Data reception
/// - Data sending
template<int sock_type>
class SocketInterface
{
  public:
    // XXX TV 18/08/2015
    // Instead of passing all these parameters, I could directly
    // pass a pointer to the session 
    SocketInterface(std::shared_ptr<KServerConfig> config_, KServer *kserver_, 
                    int comm_fd_, SessID id_)
    : config(config_), 
      kserver(kserver_),
      comm_fd(comm_fd_),
      id(id_),
      websock(config_, kserver_)
    {
        bzero(recv_data_buff, KSERVER_RECV_DATA_BUFF_LEN);
    }

    int init();
    int exit();

    int read_command(Command& cmd);
    int read_data(char *buff_str);
    int rcv_n_bytes(char *buffer, uint32_t n_bytes);
    int RcvDataBuffer(uint32_t n_bytes);
    const uint32_t* RcvHandshake(uint32_t buff_size);

    template<class T> int Send(const T& data);
    template<typename T> int Send(const std::vector<T>& vect);
    template<typename T, std::size_t N> int Send(const std::array<T, N>& vect);

    int SendCstr(const char *string);
    template<class T> int SendArray(const T *data, unsigned int len);
    template<typename... Tp> int Send(const std::tuple<Tp...>& t);

  protected:
    std::shared_ptr<KServerConfig> config;
    KServer *kserver;
    int comm_fd;
    SessID id;

  private:
    char read_str[KSERVER_READ_STR_LEN];  ///< Read string
    char recv_data_buff[KSERVER_RECV_DATA_BUFF_LEN];  ///< Receive data buffer
    WebSocket websock;
}; // Socket

template<int sock_type>
template<typename T>
int SocketInterface<sock_type>::Send(const std::vector<T>& vect)
{
    return SendArray<T>(vect.data(), vect.size());
}

template<int sock_type>
template<typename T, size_t N>
int SocketInterface<sock_type>::Send(const std::array<T, N>& vect)
{
    return SendArray<T>(vect.data(), N);
}

// http://stackoverflow.com/questions/1374468/stringstream-string-and-char-conversion-confusion
template<int sock_type>
template<typename... Tp>
int SocketInterface<sock_type>::Send(const std::tuple<Tp...>& t)
{
    std::stringstream ss;
    stringify_tuple(t, ss);
    ss << std::endl;

    const std::string& tmp = ss.str();
    return SendCstr(tmp.c_str());
}

template<> int SocketInterface<TCP>::rcv_n_bytes(char *buffer, uint32_t n_bytes);
template<> int SocketInterface<TCP>::SendCstr(const char *string);

template<> int SocketInterface<WEBSOCK>::SendCstr(const char *string);

// -----------------------------------------------
// TCP
// -----------------------------------------------

#if KSERVER_HAS_TCP

template<>
template<class T>
int SocketInterface<TCP>::SendArray(const T *data, unsigned int len)
{
    int bytes_send = sizeof(T) * len;
    int n_bytes_send = write(comm_fd, (void*)data, bytes_send);

    if (n_bytes_send < 0) {
       kserver->syslog.print(SysLog::ERROR, 
          "TCPSocket::SendArray: Can't write to client\n");
       return -1;
    }

    if (n_bytes_send != bytes_send) {
        kserver->syslog.print(SysLog::ERROR, 
            "TCPSocket::SendArray: Some bytes have not been sent\n");
        return -1;
    }

    kserver->syslog.print(SysLog::DEBUG, "[S] [%u bytes]\n", bytes_send);
    return bytes_send;
}

#endif // KSERVER_HAS_TCP

// -----------------------------------------------
// Unix socket
// -----------------------------------------------

#if KSERVER_HAS_UNIX_SOCKET
// Unix socket has the same interface than TCP socket
template<>
class SocketInterface<UNIX> : public SocketInterface<TCP>
{
  public:
    SocketInterface<UNIX>(std::shared_ptr<KServerConfig> config_,
                          KServer *kserver_, int comm_fd_, SessID id_)
    : SocketInterface<TCP>(config_, kserver_, comm_fd_, id_) {}
};
#endif // KSERVER_HAS_UNIX_SOCKET

// -----------------------------------------------
// WebSocket
// -----------------------------------------------

#if KSERVER_HAS_WEBSOCKET

template<>
template<class T>
int SocketInterface<WEBSOCK>::SendArray(const T *data, unsigned int len)
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

