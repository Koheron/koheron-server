/// Implementation of kserver.hpp
///
/// (c) Koheron

#include "kserver.hpp"

#include <chrono>
#include <cstdlib>

#include "commands.hpp"
#include "kserver_session.hpp"
#include "session_manager.hpp"
#include "syslog.tpp"

extern "C" {
  #include <sys/un.h>
}

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

bool KServer::is_ready()
{
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

#if KSERVER_HAS_SYSTEMD

#define UMIN(a, b) ((a) < (b) ? (a) : (b))

void KServer::notify_systemd_ready()
{

    /// TV (26/12/2016)
    /// Could also call sd_notify directly but did not find an
    /// easy way to cross-compile and link with libsystemd for armhf.
    ///
    /// So we call directly the notification socket as uwsgi does:
    /// https://github.com/unbit/uwsgi/blob/master/core/notify.c

    struct sockaddr_un sd_sun;
    struct msghdr msg;
    const char *state;

    int sd_notif_fd = socket(AF_UNIX, SOCK_DGRAM|SOCK_CLOEXEC, 0);

    if (sd_notif_fd < 0) {
        syslog.print<WARNING>("Cannot open notification socket\n");
        return;
    }

    int len = strlen(config->notify_socket);
    memset(&sd_sun, 0, sizeof(struct sockaddr_un));
    sd_sun.sun_family = AF_UNIX;
    strncpy(sd_sun.sun_path, config->notify_socket, UMIN(len, (int)sizeof(sd_sun.sun_path)));

    if (sd_sun.sun_path[0] == '@')
        sd_sun.sun_path[0] = 0;

    memset(&msg, 0, sizeof(struct msghdr));
    msg.msg_iov = (struct iovec*)malloc(sizeof(struct iovec) * 3);

    if (msg.msg_iov == nullptr) {
        syslog.print<WARNING>("Cannot allocate msg.msg_iov\n");
        goto exit_notification_socket;
    }

    memset(msg.msg_iov, 0, sizeof(struct iovec) * 3);
    msg.msg_name = &sd_sun;
    msg.msg_namelen = sizeof(struct sockaddr_un) - (sizeof(sd_sun.sun_path) - len);

    state = "STATUS=Koheron server is ready\nREADY=1\n";
    msg.msg_iov[0].iov_base = const_cast<char *>(state);
    msg.msg_iov[0].iov_len = strlen(state);
    msg.msg_iovlen = 1;

    if (sendmsg(sd_notif_fd, &msg, MSG_NOSIGNAL) < 0) {
        syslog.print<WARNING>("Cannot send notification to systemd.\n");
    }

    free(msg.msg_iov);

exit_notification_socket:
    if (::shutdown(sd_notif_fd, SHUT_RDWR) < 0)
        syslog.print<WARNING>("Cannot shutdown notification socket\n");

    close(sd_notif_fd);
}
#endif

int KServer::run()
{
    bool ready_notified = false;
    start_time = std::time(nullptr);

    if (start_listeners_workers() < 0)
        return -1;

    while (true) {
        if (!ready_notified && is_ready()) {
            syslog.print<INFO>("Koheron server ready\n");
#if KSERVER_HAS_SYSTEMD
            if (config->notify_systemd)
                notify_systemd_ready();
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
