/// Implementation of kserver.hpp
///
/// (c) Koheron

#include "kserver.hpp"

#include <chrono>

#include "commands.hpp"
#include "kserver_session.hpp"
#include "session_manager.hpp"

namespace kserver {

KServer::KServer(std::shared_ptr<kserver::KServerConfig> config_)
: KDevice<KServer, KSERVER>(this),
  config(config_),
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
  session_manager(*this, dev_manager, SessionManager::DFLT_WRITE_PERM_POLICY),
  syslog(config_),
  start_time(0),
  broadcast(session_manager)
{
    if (sig_handler.Init(this))
        exit(EXIT_FAILURE);

    if (dev_manager.Init() < 0)
        exit (EXIT_FAILURE);

    exit_comm.store(false);

#if KSERVER_HAS_TCP
    if (tcp_listener.init() < 0)
        exit(EXIT_FAILURE);
#else
    if (config->tcp_worker_connections > 0)
        syslog.print(SysLog::ERROR, "TCP connections not supported\n");
#endif // KSERVER_HAS_TCP

#if KSERVER_HAS_WEBSOCKET
    if (websock_listener.init() < 0)
        exit(EXIT_FAILURE);
#else
    if (config->websock_worker_connections > 0)
        syslog.print(SysLog::ERROR, "Websocket connections not supported\n");
#endif // KSERVER_HAS_WEBSOCKET

#if KSERVER_HAS_UNIX_SOCKET
    if (unix_listener.init() < 0)
        exit(EXIT_FAILURE);
#else
    if (config->unixsock_worker_connections > 0)
        syslog.print(SysLog::ERROR, "Unix socket connections not supported\n");
#endif // KSERVER_HAS_UNIX_SOCKET
}

KServer::~KServer()
{}

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

    do { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
    while ( !tcp_listener.is_closed.load()
         || !websock_listener.is_closed.load()
         || !unix_listener.is_closed.load());
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

void KServer::detach_listeners_workers()
{
#if KSERVER_HAS_TCP && KSERVER_HAS_THREADS
    tcp_listener.detach_worker();
#endif

#if KSERVER_HAS_WEBSOCKET && KSERVER_HAS_THREADS
    websock_listener.detach_worker();
#endif

#if KSERVER_HAS_UNIX_SOCKET && KSERVER_HAS_THREADS
    unix_listener.detach_worker();
#endif
}

void KServer::join_listeners_workers()
{
#if KSERVER_HAS_TCP && KSERVER_HAS_THREADS
    tcp_listener.join_worker();
#endif

#if KSERVER_HAS_WEBSOCKET && KSERVER_HAS_THREADS
    websock_listener.join_worker();
#endif

#if KSERVER_HAS_UNIX_SOCKET && KSERVER_HAS_THREADS
    unix_listener.join_worker();
#endif
}

int KServer::Run()
{
    start_time = std::time(nullptr);

    if (start_listeners_workers() < 0)
        return -1;

    while (1) {
        if (sig_handler.Interrupt()) {
            syslog.print(SysLog::INFO, 
                         "Interrupt received, killing KServer ...\n");

            detach_listeners_workers();

            syslog.print(SysLog::INFO, "Closing all active sessions ...\n");
            session_manager.DeleteAll(); 

            goto exit;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // join_listeners_workers();

exit:
    close_listeners();
    syslog.close();
    return 0;
}

} // namespace kserver
