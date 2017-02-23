/// Implementation of config.hpp
///
/// (c) Koheron

#include "config.hpp"

#include <fstream>
#include <cstring>
#include <cinttypes>
#include <string>
#include <streambuf>

#include "string_utils.hpp"

namespace kserver {

KServerConfig::KServerConfig()
: verbose(false),
  tcp_nodelay(false),
  syslog(false),
  use_stderr(false),
  daemon(true),
  notify_systemd(false),
  tcp_port(TCP_DFLT_PORT),
  tcp_worker_connections(DFLT_WORKER_CONNECTIONS),
  websock_port(WEBSOCKET_DFLT_PORT),
  websock_worker_connections(DFLT_WORKER_CONNECTIONS),
  unixsock_worker_connections(DFLT_WORKER_CONNECTIONS)
{
    memset(unixsock_path, 0, UNIX_SOCKET_PATH_LEN);
    strcpy(unixsock_path, DFLT_UNIX_SOCK_PATH);

    memset(notify_socket, 0, UNIX_SOCKET_PATH_LEN);
    strcpy(notify_socket, DFLT_NOTIFY_SOCKET);
}

char* KServerConfig::_get_source(const char *filename)
{
    std::ifstream t(filename);

    if (!t) {
        kserver::fprintf(stderr, "Cannot open file %s\n", filename);
        return nullptr;
    }

    std::string config_str;

    t.seekg(0, std::ios::end);
    config_str.reserve(t.tellg());
    t.seekg(0, std::ios::beg);

    config_str.assign((std::istreambuf_iterator<char>(t)),
                       std::istreambuf_iterator<char>());

    t.close();

    auto *source = new char[config_str.length() + 1];
    std::strcpy(source, config_str.c_str());
    return source;
}

// Check whether a field is set to "ON" or "OFF"
int is_on(JsonValue value)
{
    if (value.getTag() != JSON_STRING) {
        return -1;
    }

    if (strcmp(value.toString(), "ON") == 0) {
        return 1;
    }

    if (strcmp(value.toString(), "OFF") == 0) {
        return 0;
    }

    return -1;
}

#define READ_BOOL(name)                                        \
    int status = is_on(value);                                 \
                                                               \
    if (status < 0) {                                          \
        kserver::fprintf(stderr, "Invalid field " #name "\n"); \
        return -1;                                             \
    }                                                          \
                                                               \
    (name) = status;                                           \
    return 0;

int KServerConfig::_read_verbose(JsonValue value) {
    READ_BOOL(verbose)
}

int KServerConfig::_read_tcp_nodelay(JsonValue value) {
    READ_BOOL(tcp_nodelay)
}

int KServerConfig::_read_daemon(JsonValue value) {
    READ_BOOL(daemon)
}

int KServerConfig::_read_notify_systemd(JsonValue value) {
    READ_BOOL(notify_systemd)
}

int KServerConfig::_read_log(JsonValue value)
{
    if (value.getTag() != JSON_OBJECT) {
        kserver::fprintf(stderr, "Invalid field log\n");
        return -1;
    }

    for (auto i : value) {
        if (strcmp(i->key, "system_log") == 0) {
            int status = is_on(i->value);

            if (status < 0) {
                kserver::fprintf(stderr, "Invalid field system_log\n");
                return -1;
            }

            syslog = (status != 0);
        } else if (strcmp(i->key, "use_stderr") == 0) {
            int status = is_on(i->value);

            if (status < 0) {
                fprintf(stderr, "Invalid field system_log\n");
                return -1;
            }

            use_stderr = (status != 0);
        } else {
            kserver::fprintf(stderr, "Invalid key in log\n");
            return -1;
        }
    }

    return 0;
}

int check_unixsocket_path(const char *path)
{
    // Must be an abstract socket, or an absolute path
    if ((path[0] != '@' && path[0] != '/') || path[1] == 0) {
        kserver::fprintf(stderr, "Invalid path for unix socket %s\n", path);
        return -1;
    }

    if (strlen(path) > UNIX_SOCKET_PATH_LEN) {
        kserver::fprintf(stderr, "Unix socket path too long\n");
        return -1;
    }

    return 0;
}

int KServerConfig::_read_server(JsonValue value, server_t serv_type)
{
    if (value.getTag() != JSON_OBJECT) {
        kserver::fprintf(stderr, "Invalid TCP field\n");
        return -1;
    }

    for (auto i : value) {
        if (strcmp(i->key, "listen") == 0) {
            if (i->value.getTag() != JSON_NUMBER) {
                kserver::fprintf(stderr, "Invalid value in field listen\n");
                return -1;
            }
            
            if (serv_type == TCP_SERVER) {            
                tcp_port = i->value.toNumber();
            }
            else if (serv_type == WEBSOCK_SERVER) {
                websock_port = i->value.toNumber();
            } else { // UNIXSOCK_SERVER
                kserver::fprintf(stderr, "Unix socket don't have a listen port\n");
                return -1;
            }
        }
        else if(strcmp (i->key, "path") == 0) {
            if (serv_type != UNIXSOCK_SERVER) {
                kserver::fprintf(stderr, "Field path only valid for Unix socket\n");
                return -1;
            }

            if (i->value.getTag() != JSON_STRING) {
                kserver::fprintf(stderr, "Invalid value in field path\n");
                return -1;
            }

            const char *path = i->value.toString();

            if (check_unixsocket_path(path) < 0) {
                return -1;
            }

            strcpy(unixsock_path, path);
        }        
        else if (strcmp(i->key, "worker_connections") == 0) {
            if (i->value.getTag() != JSON_NUMBER) {
                fprintf(stderr, "Invalid value in field worker_connections\n");
                return -1;
            }

            if (serv_type == TCP_SERVER) {
                tcp_worker_connections = i->value.toNumber();
            } else if (serv_type == WEBSOCK_SERVER) {
                websock_worker_connections = i->value.toNumber();
            } else if (serv_type == UNIXSOCK_SERVER) {
                unixsock_worker_connections = i->value.toNumber();
            }
        } else {
            fprintf(stderr, "Unknown server key %s\n", i->key);
            return -1;
        }
    }

    return 0;
}

int KServerConfig::_read_tcp(JsonValue value)
{
    return _read_server(value, TCP_SERVER);
}

int KServerConfig::_read_websocket(JsonValue value)
{
    return _read_server(value, WEBSOCK_SERVER);
}

int KServerConfig::_read_unixsocket(JsonValue value)
{
    return _read_server(value, UNIXSOCK_SERVER);
}

void KServerConfig::_check_config()
{
    if (daemon) {
        // Desactivate verbose if run as a daemon
        // since stdout is not available
        verbose = false;

        // In daemon mode it is advisable to 
        // activate the syslog since this is the
        // main output for the program
        if (!syslog) {
            printf("NOTICE: You should consider to activate "
                   "the syslog when running KServer as a daemon\n");
        }
    }
}

#define TEST_KEY(key_name)          \
    strcmp(i->key, key_name) == 0

#define IS_VERBOSE         TEST_KEY("verbose")
#define IS_TCP_NODELAY     TEST_KEY("tcp_nodelay")
#define IS_DAEMON          TEST_KEY("daemon")
#define IS_NOTIFY_SYSTEMD  TEST_KEY("notify-systemd")
#define IS_NOTIFY_SOCKET   TEST_KEY("notify-socket")
#define IS_LOGS            TEST_KEY("logs")
#define IS_TCP             TEST_KEY("TCP")
#define IS_WEBSOCKET       TEST_KEY("websocket")
#define IS_UNIX            TEST_KEY("unix")

int KServerConfig::load_file(const char *filename)
{
    char *source = _get_source(filename);

    if (source == nullptr) {
        return -1;
}

    char *endptr;
    JsonValue value;
    JsonAllocator allocator{};

    int status = jsonParse(source, &endptr, &value, allocator);

    if (status != JSON_OK) {
        fprintf(stderr, "JSON Error: %s at %zd\n",
                jsonStrError(status), endptr - source);
        return -1;
    }

    if (value.getTag() != JSON_OBJECT) {
        kserver::fprintf(stderr, "Invalid configuration file\n");
        return -1;
    }

    for (auto i : value) {
        if (IS_VERBOSE) {
            if (_read_verbose(i->value) < 0) {
                return -1;
            }
        }
        else if (IS_TCP_NODELAY) {
            if (_read_tcp_nodelay(i->value) < 0) {
                return -1;
            }
        }
        else if (IS_DAEMON) {
            if (_read_daemon(i->value) < 0) {
                return -1;
            }
        }
        else if (IS_NOTIFY_SYSTEMD) {
            if (_read_notify_systemd(i->value) < 0) {
                return -1;
            }
        }
        else if (IS_NOTIFY_SOCKET) {
            if (i->value.getTag() != JSON_STRING) {
                kserver::fprintf(stderr, "Invalid value in field notify-socket\n");
                return -1;
            }

            const char *notify_socket_path = i->value.toString();

            if (check_unixsocket_path(notify_socket_path) < 0) {
                return -1;
            }

            strcpy(unixsock_path, notify_socket_path);
        }
        else if (IS_LOGS) {
            if (_read_log(i->value) < 0) {
                return -1;
            }
        }
        else if (IS_TCP) {
            if (_read_tcp(i->value) < 0) {
                return -1;
            }
        }
        else if (IS_WEBSOCKET) {
            if (_read_websocket(i->value) < 0) {
                return -1;
            }
        }
        else if (IS_UNIX) {
            if (_read_unixsocket(i->value) < 0) {
                return -1;
            }
        } else {
            fprintf(stderr, "Unknown field %s in configuration file\n", i->key);
            return -1;
        }
    }

    if (verbose) {
        show();
    }

    _check_config();
    delete[] source;
    return 0;
}

void KServerConfig::show()
{
    printf("\n==================\n");
    printf("  KServer config  \n");
    printf("==================\n\n");

    printf("Verbose: %s\n", verbose ? "ON": "OFF");
    printf("Notify systemd: %s\n", notify_systemd ? "ON": "OFF");
    printf("Systemd notification socket: %s\n", notify_socket);
    printf("System log: %s\n", syslog ? "ON": "OFF");
    printf("Use stderr: %s\n\n", use_stderr ? "ON": "OFF");

    printf("TCP listen: %u\n", tcp_port);
    printf("TCP workers: %i\n\n", tcp_worker_connections);

    printf("Websocket listen: %u\n", websock_port);
    printf("Websocket workers: %i\n\n", websock_worker_connections);

    printf("Unix socket path: %s\n", unixsock_path);
    printf("Unix socket workers: %i\n\n", unixsock_worker_connections);
}

} // namespace kserver
