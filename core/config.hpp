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
    KServerConfig();

    /// Load the config from a JSON configuration file
    int load_file(char *filename);

    void show();

    /// Display messages emitted and received
    bool verbose;

    /// Enable/Disable the Nagle algorithm in the TCP buffer
    bool tcp_nodelay;

    /// Run KServer as a daemon if true
    bool daemon;

    /// Notify systemd when server ready
    bool notify_systemd;

    /// Notification socket for systemd
    char notify_socket[UNIX_SOCKET_PATH_LEN];

    /// TCP listening port
    unsigned int tcp_port;
    /// TCP max parallel connections
    unsigned int tcp_worker_connections;

    /// Websocket listening port
    unsigned int websock_port;
    /// Websocket max parallel connections
    unsigned int websock_worker_connections;

    /// Unix socket file path
    char unixsock_path[UNIX_SOCKET_PATH_LEN];
    /// Unix socket max parallel connections
    unsigned int unixsock_worker_connections;

  private:
    char* _get_source(char *filename);

    // Performs some checks and tuning on the loaded configuration
    void _check_config();

    int _read_verbose(JsonValue value);
    int _read_tcp_nodelay(JsonValue value);
    int _read_daemon(JsonValue value);
    int _read_notify_systemd(JsonValue value);
    int _read_server(JsonValue value, server_t serv_type);
    int _read_tcp(JsonValue value);
    int _read_websocket(JsonValue value);
    int _read_unixsocket(JsonValue value);
};

} // namespace kserver

#endif // __CONFIG_HPP__
