/// @file peer_info.cpp
///
/// @brief Implementation of peer_info.hpp
///
/// @author Thomas Vanderbruggen <thomas@koheron.com>
/// @date 30/11/2014
///
/// (c) Koheron 2014

#include "peer_info.hpp"

namespace kserver {

// --------------------------------
// PeerInfo
// --------------------------------

void PeerInfo::__build(struct sockaddr* sock_)
{
    if(sock_ == nullptr) {
        ip_family = 0;
        strcpy(ip_str,"");
        port = 0;
    } else {        
        ip_family = sock_->sa_family;

        if(ip_family == AF_INET) {
            struct sockaddr_in *s = (struct sockaddr_in *)&sock_;
            port = ntohs(s->sin_port);
            inet_ntop(AF_INET, &s->sin_addr, ip_str, sizeof ip_str);
        } else { // AF_INET6
            struct sockaddr_in6 *s = (struct sockaddr_in6 *)&sock_;
            port = ntohs(s->sin6_port);
            inet_ntop(AF_INET6, &s->sin6_addr, ip_str, sizeof ip_str);
        }
    }
}

PeerInfo::PeerInfo(int comm_fd)
{
    memset(ip_str, 0, INET6_ADDRSTRLEN);

    if(comm_fd == -1) {
        __build(nullptr);
    } else {
        // Get client informations
        struct sockaddr_storage addr;
        socklen_t len = sizeof addr;
        struct sockaddr *sockaddr_ptr;

        if(getpeername(comm_fd, (struct sockaddr*)&addr, &len) < 0) {
            sockaddr_ptr = nullptr;
        } else {
            sockaddr_ptr = (struct sockaddr*)&addr;
        }
	        
        __build(sockaddr_ptr);
    }
}

PeerInfo::PeerInfo(const PeerInfo& peer_info)
{
    ip_family = peer_info.ip_family;
    strcpy(ip_str, peer_info.ip_str);
    port = peer_info.port;
}

} // namespace kserver

