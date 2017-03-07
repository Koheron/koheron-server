/// Server definitions and static configurations
///
/// (c) Koheron

#ifndef __KSERVER_DEFS_HPP__
#define __KSERVER_DEFS_HPP__

#include <cstdint>

namespace kserver {

/// Pending connections queue size
#define KSERVER_BACKLOG 10

// ------------------------------------------
// Buffer sizes
// ------------------------------------------

/// Number of samples
#define KSERVER_SIG_LEN 16384

/// Command payload buffer length
constexpr int64_t CMD_PAYLOAD_BUFFER_LEN = 16384 * 8;

/// Read string length
#define KSERVER_READ_STR_LEN 16384

/// Send string length
#define KSERVER_SEND_STR_LEN 16384

/// Receive data buffer length
#define KSERVER_RECV_DATA_BUFF_LEN 16384 * 2 * 4

/// Websocket receive buffer size
#define WEBSOCK_READ_STR_LEN KSERVER_RECV_DATA_BUFF_LEN

/// Websocket send buffer size (bytes)
#define WEBSOCK_SEND_BUF_LEN 16384 * 2 * 4

/// Maximum length of the Unix socket file path
/// http://man7.org/linux/man-pages/man7/unix.7.html
#define UNIX_SOCKET_PATH_LEN 108

// ------------------------------------------
// Misc
// ------------------------------------------

typedef int SessID;

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

} // namespace kserver

#endif // __KSERVER_DEFS_HPP__


