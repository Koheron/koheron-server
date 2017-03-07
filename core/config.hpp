/// Server configuration
///
/// Load and store configuration file content
///
/// (c) Koheron

#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

namespace kserver {
    namespace config {
        /// Display messages emitted and received
        bool verbose = false;

        /// Enable/Disable the Nagle algorithm in the TCP buffer
        bool tcp_nodelay = true;

        /// TCP listening port
        unsigned int tcp_port = 36000;
        /// TCP max parallel connections
        unsigned int tcp_worker_connections = 100;

        /// Websocket listening port
        unsigned int websock_port = 8080;
        /// Websocket max parallel connections
        unsigned int websock_worker_connections = 100;

        /// Unix socket file path
        char unixsock_path[UNIX_SOCKET_PATH_LEN] = "/tmp/koheron-server.sock";
        /// Unix socket max parallel connections
        unsigned int unixsock_worker_connections = 100;
    }
} // namespace kserver

#endif // __CONFIG_HPP__
