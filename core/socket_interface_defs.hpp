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
    TCP,
    WEBSOCK,
    UNIX,
    sock_type_num
};

/// Listening channel descriptions
const std::array<std::string, sock_type_num>
listen_channel_desc = {{
    "NONE",
    "TCP",
    "WebSocket",
    "Unix socket"
}};

} // namespace kserver

#endif // __SOCKET_INTERFACE_DEFS_HPP__