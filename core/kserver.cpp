/// Implementation of kserver.hpp
///
/// (c) Koheron

#include "kserver.hpp"

#include <chrono>

#include "commands.hpp"
#include "kserver_session.hpp"
#include "session_manager.hpp"

extern "C" {
  #include <sys/un.h>
  #include <sys/socket.h>
}

namespace kserver {

KServer::KServer(std::shared_ptr<kserver::KServerConfig> config_)
: config(config_),
  sig_handler(),
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
        exit(EXIT_FAILURE);
    }
    if (websock_listener.init() < 0) {
        exit(EXIT_FAILURE);
    }
    if (unix_listener.init() < 0) {
        exit(EXIT_FAILURE);
    }

}

// This cannot be done in the destructor
// since it is called after the "delete config"
// at the end of the main()
void KServer::close_listeners()
{
    exit_comm.store(true);
    assert(config != NULL);
    tcp_listener.shutdown();
    websock_listener.shutdown();
    unix_listener.shutdown();
    join_listeners_workers();
}

int KServer::start_listeners_workers()
{
    if (tcp_listener.start_worker() < 0) {
        return -1;
    }
    if (websock_listener.start_worker() < 0) {
        return -1;
    }
    if (unix_listener.start_worker() < 0) {
        return -1;
    }
    return 0;
}

void KServer::join_listeners_workers()
{
    tcp_listener.join_worker();
    websock_listener.join_worker();
    unix_listener.join_worker();
}

bool KServer::is_ready()
{
    bool ready = true;

    if (config->tcp_worker_connections > 0) {
        ready = ready && tcp_listener.is_ready;
    }
    if (config->websock_worker_connections > 0) {
        ready = ready && websock_listener.is_ready;
    }
    if (config->unixsock_worker_connections > 0) {
        ready = ready && unix_listener.is_ready;
    }
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
        goto exit_notification_socket;
    }

    memset(msg.msg_iov, 0, sizeof(struct iovec) * 3);
    msg.msg_name = &sd_sun;
    msg.msg_namelen = sizeof(struct sockaddr_un) - (sizeof(sd_sun.sun_path) - len);

    state = "STATUS=Koheron server is ready\nREADY=1\n";
    msg.msg_iov[0].iov_base = (char *)(state);
    msg.msg_iov[0].iov_len = strlen(state);
    msg.msg_iovlen = 1;

    if (sendmsg(sd_notif_fd, &msg, MSG_NOSIGNAL) < 0) {
    }

    free(msg.msg_iov);

exit_notification_socket:
    ::shutdown(sd_notif_fd, SHUT_RDWR);
    close(sd_notif_fd);
}
#endif

int KServer::run()
{
    bool ready_notified = false;

    if (start_listeners_workers() < 0)
        return -1;

    while (1) {
        if (!ready_notified && is_ready()) {
#if KSERVER_HAS_SYSTEMD
            if (config->notify_systemd)
                notify_systemd_ready();
#endif
            ready_notified = true;
        }

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
