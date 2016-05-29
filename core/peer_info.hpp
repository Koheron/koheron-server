///  Client infos
///
/// (c) Koheron

#ifndef __PEER_INFO_HPP__
#define __PEER_INFO_HPP__

#include <cstring>

extern "C" {
  #include <sys/socket.h>	// socket definitions
  #include <sys/types.h>	// socket types      
  #include <arpa/inet.h>	// inet (3) funtions
}

namespace kserver {

struct PeerInfo
{
    PeerInfo(int comm_fd = -1);
    PeerInfo(const PeerInfo& peer_info);

    unsigned short ip_family;       ///< Address familly, AF_XXX
    char ip_str[INET6_ADDRSTRLEN];  ///< IP address
    int port;                       ///< Connection port
};

} // namespace kserver

#endif // __PEER_INFO_HPP__
