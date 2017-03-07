/// Implementation of kserver.hpp
///
/// (c) Koheron

#include "kserver.hpp"

#include <chrono>

#include "commands.hpp"
#include "kserver_session.hpp"
#include "session_manager.hpp"
#include "config.hpp"

extern "C" {
  #include <sys/un.h>
  #include <sys/socket.h>
}

namespace kserver {

KServer::KServer()
: sig_handler(),
  tcp_listener(this),
  websock_listener(this),
  unix_listener(this),
  dev_manager(this),
  session_manager(*this, dev_manager)
{
    if (sig_handler.init(this) < 0) {
        exit(EXIT_FAILURE);
    }
    if (dev_manager.init() < 0) {
        exit (EXIT_FAILURE);
    }
    exit_comm.store(false);
    exit_all.store(false);

    if (tcp_listener.init() < 0) {
        printf("Failed to initialize TCP socket\n");
        exit(EXIT_FAILURE);
    }
    if (websock_listener.init() < 0) {
        printf("Failed to initialize Webocket\n");
        exit(EXIT_FAILURE);
    }
    if (unix_listener.init() < 0) {
        printf("Failed to initialize Unix socket\n");
        exit(EXIT_FAILURE);
    }
}

void KServer::close_listeners()
{
    exit_comm.store(true);
    tcp_listener.shutdown();
    websock_listener.shutdown();
    unix_listener.shutdown();
    join_listeners_workers();
}

int KServer::start_listeners_workers()
{
    if (tcp_listener.start_worker() < 0) {
        printf("Failed to start TCP socket worker\n");
        return -1;
    }
    if (websock_listener.start_worker() < 0) {
        printf("Failed to start Websocket worker\n");
        return -1;
    }
    if (unix_listener.start_worker() < 0) {
        printf("Failed to start Unix socket worker\n");
        return -1;
    }
    return 0;
}

void KServer::join_listeners_workers() {
    tcp_listener.join_worker();
    websock_listener.join_worker();
    unix_listener.join_worker();
}

int KServer::run() {

    printf("KServer::run()\n");

    if (start_listeners_workers() < 0) {
        printf("Failed to start listeners \n");
        return -1;
    }

    while (1) {
        if (sig_handler.interrupt() || exit_all) {
            session_manager.delete_all();
            close_listeners();
            return 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return 0;
}

} // namespace kserver
