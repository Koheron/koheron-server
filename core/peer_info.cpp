/// (c) Koheron

#include "peer_info.hpp"

namespace kserver {

PeerInfo::PeerInfo(int comm_fd)
{
    memset(ip_str, 0, INET6_ADDRSTRLEN);
    ip_family = 0;
    port = 0;

    if (comm_fd == -1)
        return;

    struct sockaddr_storage addr;
    socklen_t len = sizeof addr;

    if (getpeername(comm_fd, (struct sockaddr*)&addr, &len) >= 0) {
        ip_family = addr.ss_family;
        port = ntohs(((struct sockaddr_in *)&addr)->sin_port);
        inet_ntop(ip_family , &(((struct sockaddr_in *)&addr)->sin_addr), ip_str, sizeof ip_str);
    }
}

PeerInfo::PeerInfo(const PeerInfo& peer_info)
{
    ip_family = peer_info.ip_family;
    strcpy(ip_str, peer_info.ip_str);
    port = peer_info.port;
}

} // namespace kserver

