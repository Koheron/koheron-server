/// Implementation of config.hpp
///
/// (c) Koheron

#include <fstream>
#include <string>
#include <cstring>
#include <streambuf>
#include <inttypes.h>

#include "config.hpp"

namespace kserver {

KServerConfig::KServerConfig()
: verbose(false),
  tcp_nodelay(false),
  syslog(false),
  daemon(true),
  tcp_port(TCP_DFLT_PORT),
  tcp_worker_connections(DFLT_WORKER_CONNECTIONS),
  websock_port(WEBSOCKET_DFLT_PORT),
  websock_worker_connections(DFLT_WORKER_CONNECTIONS),
  unixsock_worker_connections(DFLT_WORKER_CONNECTIONS),
  addr_limit_down(DFLT_ADDR_LIMIT_DOWN),
  addr_limit_up(DFLT_ADDR_LIMIT_UP)
{
   memset(unixsock_path, 0, UNIX_SOCKET_PATH_LEN);
   strcpy(unixsock_path, DFLT_UNIX_SOCK_PATH);
}

char* KServerConfig::_get_source(char *filename)
{
    std::ifstream t(filename);

    if (!t) {
        fprintf(stderr, "Cannot open file %s\n", filename);
        return nullptr;
    }

    std::string config_str;

    t.seekg(0, std::ios::end);
    config_str.reserve(t.tellg());
    t.seekg(0, std::ios::beg);

    config_str.assign((std::istreambuf_iterator<char>(t)),
                       std::istreambuf_iterator<char>());

    t.close();

    char *source = new char[config_str.length() + 1];
    std::strcpy(source, config_str.c_str());
    return source;
}

// Check whether a field is set to "ON" or "OFF"
int is_on(JsonValue value)
{
    if (value.getTag() != JSON_STRING)
        return -1;

    if (strcmp(value.toString(), "ON") == 0)
        return 1;
    else if (strcmp(value.toString(), "OFF") == 0)
        return 0;
    else
        return -1;
}

int KServerConfig::_read_verbose(JsonValue value)
{
    int status = is_on(value);

    if (status < 0) {
        fprintf(stderr, "Invalid field verbose\n");
        return -1;
    }

    verbose = status;
    return 0;
}

int KServerConfig::_read_tcp_nodelay(JsonValue value)
{
    int status = is_on(value);

    if (status < 0) {
        fprintf(stderr, "Invalid field tcp_nodelay\n");
        return -1;
    }

    tcp_nodelay = status;
    return 0;
}

int KServerConfig::_read_daemon(JsonValue value)
{
    int status = is_on(value);

    if (status < 0) {
        fprintf(stderr, "Invalid field daemon\n");
        return -1;
    }

    daemon = status;
    return 0;
}

int KServerConfig::_read_log(JsonValue value)
{
    if (value.getTag() != JSON_OBJECT) {
        fprintf(stderr, "Invalid field log\n");
        return -1;
    }

    for (auto i : value) {
        if (strcmp(i->key, "system_log") == 0) {
            int status = is_on(i->value);

            if (status < 0) {
                fprintf(stderr, "Invalid field system_log\n");
                return -1;
            }

            syslog = status;
        } else {
            fprintf(stderr, "Invalid key in log\n");
            return -1;
        }
    }

    return 0;
}

int KServerConfig::_read_server(JsonValue value, server_t serv_type)
{
    if (value.getTag() != JSON_OBJECT) {
        fprintf(stderr, "Invalid TCP field\n");
        return -1;
    }

    for (auto i : value) {
        if (strcmp(i->key, "listen") == 0) {
            if (i->value.getTag() != JSON_NUMBER) {
                fprintf(stderr, "Invalid value in field listen\n");
                return -1;
            }
            
            if (serv_type == TCP_SERVER) {            
                tcp_port = i->value.toNumber();
            }
            else if (serv_type == WEBSOCK_SERVER) {
                websock_port = i->value.toNumber();
            } else { // UNIXSOCK_SERVER
                fprintf(stderr, "Unix socket don't have a listen port\n");
                return -1;
            }
        }
        else if(strcmp (i->key, "path") == 0) {
            if(serv_type != UNIXSOCK_SERVER) {
                fprintf(stderr, "Field path only valid for Unix socket\n");
                return -1;
            }

            if (i->value.getTag() != JSON_STRING) {
                fprintf(stderr, "Invalid value in field path\n");
                return -1;
            }

            memset(unixsock_path, 0, UNIX_SOCKET_PATH_LEN);
            strcpy(unixsock_path, i->value.toString());
        }        
        else if (strcmp(i->key, "worker_connections") == 0) {
            if (i->value.getTag() != JSON_NUMBER) {
                fprintf(stderr, "Invalid value in field worker_connections\n");
                return -1;
            }

            if (serv_type == TCP_SERVER)
                tcp_worker_connections = i->value.toNumber();
            else if (serv_type == WEBSOCK_SERVER)         
                websock_worker_connections = i->value.toNumber();
            else if (serv_type == UNIXSOCK_SERVER)        
                unixsock_worker_connections = i->value.toNumber();
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

int KServerConfig::_read_addr_limits(JsonValue value)
{
    if (value.getTag() != JSON_OBJECT) {
        fprintf(stderr, "Invalid addr_limits field\n");
        return -1;
    }

    for (auto i : value) {
        if (strcmp(i->key, "up") == 0) {
            if (i->value.getTag() != JSON_STRING) {
                fprintf(stderr, "Address limit up must be a string "
                                "of the hexadecimal address number\n");
                return -1;
            }

            addr_limit_up = (intptr_t)strtol(i->value.toString(), NULL, 0);
        }
        else if (strcmp(i->key, "down") == 0) {
            if (i->value.getTag() != JSON_STRING) {
                fprintf(stderr, "Address limit down must be a string "
                                "of the hexadecimal address number\n");
                return -1;
            }

            addr_limit_down = (intptr_t)strtol(i->value.toString(), NULL, 0);
        }
        else {
            fprintf(stderr, "Unknown addr_limits key %s\n", i->key);
            return -1;
        }
    }

    return 0;
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
        if (!syslog)
            printf("NOTICE: You should consider to activate "
                   "the syslog when running KServer as a daemon\n");
    }
}

#define TEST_KEY(key_name)          \
    strcmp(i->key, key_name) == 0

#define IS_VERBOSE      TEST_KEY("verbose")
#define IS_TCP_NODELAY  TEST_KEY("tcp_nodelay")
#define IS_DAEMON       TEST_KEY("daemon")
#define IS_LOGS         TEST_KEY("logs")
#define IS_TCP          TEST_KEY("TCP")
#define IS_WEBSOCKET    TEST_KEY("websocket")
#define IS_UNIX         TEST_KEY("unix")
#define IS_ADDR_LIMITS  TEST_KEY("addr_limits")

int KServerConfig::load_file(char *filename)
{
    char *source = _get_source(filename);

    if (source == nullptr)
        return -1;

    char *endptr;
    JsonValue value;
    JsonAllocator allocator;

    int status = jsonParse(source, &endptr, &value, allocator);

    if (status != JSON_OK) {
        fprintf(stderr, "JSON Error: %s at %zd\n",
                jsonStrError(status), endptr - source);
        return -1;
    }

    if (value.getTag() != JSON_OBJECT) {
        fprintf(stderr, "Invalid configuration file\n");
        return -1;
    }

    for (auto i : value) {
        if (IS_VERBOSE) {
            if (_read_verbose(i->value) < 0)
                return -1;
        }
        else if (IS_TCP_NODELAY) {
            if (_read_tcp_nodelay(i->value) < 0)
                return -1;
        }
        else if (IS_DAEMON) {
            if (_read_daemon(i->value) < 0)
                return -1;
        }
        else if (IS_LOGS) {
            if (_read_log(i->value) < 0)
                return -1;
        }
        else if (IS_TCP) {
            if (_read_tcp(i->value) < 0)
                return -1;
        }
        else if (IS_WEBSOCKET) {
            if (_read_websocket(i->value) < 0)
                return -1;
        }
        else if (IS_UNIX) {
            if (_read_unixsocket(i->value) < 0)
                return -1;
        }
        else if (IS_ADDR_LIMITS) {
            if (_read_addr_limits(i->value) < 0)
                return -1;
        } else {
            fprintf(stderr, "Unknown field %s in configuration file\n", i->key);
            return -1;
        }
    }

    if (verbose)
        show();

    _check_config();
    delete[] source;
    return 0;
}

void KServerConfig::show()
{
    printf("\n==================\n");
    printf("  KServer config  \n");
    printf("==================\n\n");

    printf("Verbose: %s\n\n", verbose ? "ON": "OFF");
    printf("System log: %s\n\n", syslog ? "ON": "OFF");

    printf("TCP listen: %u\n", tcp_port);
    printf("TCP workers: %u\n\n", tcp_worker_connections);

    printf("Websocket listen: %u\n", websock_port);
    printf("Websocket workers: %u\n\n", websock_worker_connections);

    // http://stackoverflow.com/questions/5795978/string-format-for-intptr-t-and-uintptr-t
    printf("Addr limit down: %" PRIxPTR "\n", addr_limit_down);
    printf("Addr limit up: %" PRIxPTR "\n\n", addr_limit_up);
    printf("\n====================================\n\n");
}

} // namespace kserver
