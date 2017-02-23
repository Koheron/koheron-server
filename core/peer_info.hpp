///  Client infos
///
/// (c) Koheron

#ifndef __PEER_INFO_HPP__
#define __PEER_INFO_HPP__

#include <cstring>

extern "C" {
  #include <sys/socket.h>   // socket definitions
  #include <sys/types.h>    // socket types
  #include <arpa/inet.h>    // inet (3) funtions
}

#include "socket_interface_defs.hpp"

namespace kserver {

template<int sock_type>
struct PeerInfo
{
    explicit PeerInfo(int comm_fd)
    {
        memset(ip_str, 0, INET6_ADDRSTRLEN);
        ip_family = 0;
        port = 0;

        if (sock_type == WEBSOCK || sock_type == TCP)
            fill_up(comm_fd);
    }

    PeerInfo(const PeerInfo& peer_info)
    {
        ip_family = peer_info.ip_family;
        strcpy(ip_str, peer_info.ip_str);
        port = peer_info.port;
    }

    uint16_t ip_family;             ///< Address family, AF_XXX
    char ip_str[INET6_ADDRSTRLEN];  ///< IP address
    int port;                       ///< Connection port

  private:
    void fill_up(int comm_fd)
    {
        struct sockaddr_storage addr{};
        socklen_t len = sizeof addr;

        if (getpeername(comm_fd, reinterpret_cast<struct sockaddr*>(&addr), &len) >= 0) {
            ip_family = addr.ss_family;
            port = ntohs(reinterpret_cast<struct sockaddr_in *>(&addr)->sin_port);
            inet_ntop(ip_family , &(reinterpret_cast<struct sockaddr_in *>(&addr)->sin_addr),
                      ip_str, sizeof ip_str);
        }
    }
};

} // namespace kserver

#endif // __PEER_INFO_HPP__
