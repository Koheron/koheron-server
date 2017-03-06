/// Server configuration
///
/// Load and store configuration file content
///
/// (c) Koheron

#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <cstdint>

#include "kserver_defs.hpp"
#include "gason.hpp"

namespace kserver {

typedef enum {
    TCP_SERVER,
    WEBSOCK_SERVER,
    UNIXSOCK_SERVER,
    server_t_num
} server_t;

struct KServerConfig
{
    /// Display messages emitted and received
    bool verbose = false;

    /// Enable/Disable the Nagle algorithm in the TCP buffer
    bool tcp_nodelay = true;

    /// Run KServer as a daemon if true
    bool daemon = true;

    /// Notify systemd when server ready
    bool notify_systemd = true;

    /// Notification socket for systemd
    char notify_socket[UNIX_SOCKET_PATH_LEN] = "/run/systemd/notify";

    /// TCP listening port
    unsigned int tcp_port = 36000;
    /// TCP max parallel connections
    unsigned int tcp_worker_connections = 100;

    /// Websocket listening port
    unsigned int websock_port = 8080;
    /// Websocket max parallel connections
    unsigned int websock_worker_connections = 100;

    /// Unix socket file path
    char unixsock_path[UNIX_SOCKET_PATH_LEN] = "/var/run/koheron-server.sock";
    /// Unix socket max parallel connections
    unsigned int unixsock_worker_connections = 100;

};

} // namespace kserver

#endif // __CONFIG_HPP__
