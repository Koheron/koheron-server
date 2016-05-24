/// Socket interface definitions
///
/// (c) Koheron

#ifndef __SOCKET_INTERFACE_DEFS_HPP__
#define __SOCKET_INTERFACE_DEFS_HPP__

#include <array>
#include <string>

#include "kserver_defs.hpp"

namespace kserver {

  /// Listening channel types
enum SockType {
    NONE,
#if KSERVER_HAS_TCP
    TCP,
#endif
#if KSERVER_HAS_WEBSOCKET
    WEBSOCK,
#endif
#if KSERVER_HAS_UNIX_SOCKET
    UNIX,
#endif
    sock_type_num
};

/// Listening channel descriptions
const std::array< std::string, sock_type_num > 
listen_channel_desc = {{
    "NONE",
#if KSERVER_HAS_TCP
    "TCP",
#endif
#if KSERVER_HAS_WEBSOCKET
    "WebSocket",
#endif
#if KSERVER_HAS_UNIX_SOCKET
    "Unix socket"
#endif
}};

} // namespace kserver

#endif // __SOCKET_INTERFACE_DEFS_HPP__