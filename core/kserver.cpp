/// Implementation of kserver.hpp
///
/// (c) Koheron

#include "kserver.hpp"

#include <chrono>

#include "commands.hpp"
#include "kserver_session.hpp"
#include "session_manager.hpp"
#include "syslog.tpp"

namespace kserver {

KServer::KServer(std::shared_ptr<kserver::KServerConfig> config_)
: config(config_),
  sig_handler(),
#if KSERVER_HAS_TCP
    tcp_listener(this),
#endif
#if KSERVER_HAS_WEBSOCKET
    websock_listener(this),
#endif
#if KSERVER_HAS_UNIX_SOCKET
    unix_listener(this),
#endif
  dev_manager(this),
  session_manager(*this, dev_manager),
  syslog(config_, sig_handler, session_manager),
  start_time(0)
{
    if (sig_handler.init(this) < 0)
        exit(EXIT_FAILURE);

    if (dev_manager.init() < 0)
        exit (EXIT_FAILURE);

    exit_comm.store(false);
    exit_all.store(false);

#if KSERVER_HAS_TCP
    if (tcp_listener.init() < 0)
        exit(EXIT_FAILURE);
#else
    if (config->tcp_worker_connections > 0)
        syslog.print<ERROR>("TCP connections not supported\n");
#endif // KSERVER_HAS_TCP

#if KSERVER_HAS_WEBSOCKET
    if (websock_listener.init() < 0)
        exit(EXIT_FAILURE);
#else
    if (config->websock_worker_connections > 0)
        syslog.print<ERROR>("Websocket connections not supported\n");
#endif // KSERVER_HAS_WEBSOCKET

#if KSERVER_HAS_UNIX_SOCKET
    if (unix_listener.init() < 0)
        exit(EXIT_FAILURE);
#else
    if (config->unixsock_worker_connections > 0)
        syslog.print<ERROR>("Unix socket connections not supported\n");
#endif // KSERVER_HAS_UNIX_SOCKET
}

// This cannot be done in the destructor
// since it is called after the "delete config"
// at the end of the main()
void KServer::close_listeners()
{
    exit_comm.store(true);
    assert(config != NULL);

#if KSERVER_HAS_TCP
    tcp_listener.shutdown();
#endif
#if KSERVER_HAS_WEBSOCKET
    websock_listener.shutdown();
#endif
#if KSERVER_HAS_UNIX_SOCKET
    unix_listener.shutdown();
#endif

#if KSERVER_HAS_THREADS
    join_listeners_workers();
#endif
}

int KServer::start_listeners_workers()
{
#if KSERVER_HAS_TCP
    if (tcp_listener.start_worker() < 0)
        return -1;
#endif
#if KSERVER_HAS_WEBSOCKET
    if (websock_listener.start_worker() < 0)
        return -1;
#endif
#if KSERVER_HAS_UNIX_SOCKET
    if (unix_listener.start_worker() < 0)
        return -1;
#endif

    return 0;
}

void KServer::join_listeners_workers()
{
#if KSERVER_HAS_TCP
    tcp_listener.join_worker();
#endif
#if KSERVER_HAS_WEBSOCKET
    websock_listener.join_worker();
#endif
#if KSERVER_HAS_UNIX_SOCKET
    unix_listener.join_worker();
#endif
}

bool KServer::is_ready() {
    bool ready = true;

#if KSERVER_HAS_TCP
    if (config->tcp_worker_connections > 0)
        ready = ready && tcp_listener.is_ready;
#endif
#if KSERVER_HAS_WEBSOCKET
    if (config->websock_worker_connections > 0)
        ready = ready && websock_listener.is_ready;
#endif
#if KSERVER_HAS_UNIX_SOCKET
    if (config->unixsock_worker_connections > 0)
        ready = ready && unix_listener.is_ready;
#endif

    return ready;
}

int KServer::run()
{
    bool ready_notified = false;
    start_time = std::time(nullptr);

    if (start_listeners_workers() < 0)
        return -1;

    while (1) {
        if (!ready_notified && is_ready()) {
            syslog.print<INFO>("Koheron server ready\n");
#if KSERVER_HAS_SYSTEMD
            // TODO notify systemd
#endif
            ready_notified = true;
        }

        if (sig_handler.interrupt() || exit_all) {
            syslog.print<INFO>("Interrupt received, killing Koheron server ...\n");
            session_manager.delete_all();
            close_listeners();
            syslog.close();
            return 0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return 0;
}

} // namespace kserver
