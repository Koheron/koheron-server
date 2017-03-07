/// Implementation and specializations of
/// the class ListeningChannel in kserver.hpp
///
/// (c) Koheron

#include "kserver.hpp"
#include "kserver_session.hpp"
#include "config.hpp"
#include <thread>
#include <atomic>
#include <unistd.h>

extern "C" {
  #include <sys/socket.h>   // socket definitions
  #include <sys/types.h>    // socket types
  #include <arpa/inet.h>    // inet (3) functions
  #include <netinet/tcp.h>
  #include <sys/un.h>
}

namespace kserver {

int create_tcp_listening(unsigned int port) {
    int listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);

    if (listen_fd_ < 0) {
        return -1;
    }

    // To avoid binding error
    // See http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#bind
    int yes = 1;
    if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1) {
        printf("Binding error");
    }

    int one = 1;
    setsockopt(listen_fd_, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one));

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(port);

    // Assign name (address) to socket
    if (bind(listen_fd_, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        close(listen_fd_);
        return -1;
    }

    printf("TCP listening successful \n");
    return listen_fd_;
}

int set_comm_sock_opts(int comm_fd) {
    int sndbuf_len = sizeof(uint32_t) * KSERVER_SIG_LEN;

    if (setsockopt(comm_fd, SOL_SOCKET, SO_SNDBUF,
                  &sndbuf_len, sizeof(sndbuf_len)) < 0) {
        close(comm_fd);
        return -1;
    }

    int rcvbuf_len = KSERVER_READ_STR_LEN;

    if (setsockopt(comm_fd, SOL_SOCKET, SO_RCVBUF,
                  &rcvbuf_len, sizeof(rcvbuf_len)) < 0) {
        close(comm_fd);
        return -1;
    }

    int one = 1;
    if (setsockopt(comm_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one)) < 0) {
        close(comm_fd);
        return -1;
    }


    return 0;
}

int open_tcp_communication(int listen_fd) {
    printf("start open_tcp_communication\n");
    int comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL);

    if (comm_fd < 0)
        return comm_fd;

    if (set_comm_sock_opts(comm_fd) < 0)
        return -1;

    printf("end open_tcp_communication\n");
    return comm_fd;
}

template<int sock_type>
void session_thread_call(int comm_fd, ListeningChannel<sock_type> *listener) {
    printf("start session_thread_call\n");
    listener->inc_thread_num();

    SessID sid = listener->kserver->session_manager. template create_session<sock_type>(comm_fd);

    listener->kserver->session_manager.delete_session(sid);

    listener->dec_thread_num();
    printf("end session_thread_call\n");
}

template<int sock_type>
void comm_thread_call(ListeningChannel<sock_type> *listener) {
    printf("start comm_thread_call\n");
    listener->is_ready = true;

    while (!listener->kserver->exit_comm.load()) {
        int comm_fd = listener->open_communication();

        if (listener->kserver->exit_comm.load())
            break;

        if (comm_fd < 0) {
            continue;
        }

        if (listener->is_max_threads()) {
            continue;
        }

        std::thread sess_thread(session_thread_call<sock_type>, comm_fd, listener);
        sess_thread.detach();
    }
    printf("end comm_thread_call\n");

}

template<int sock_type>
int ListeningChannel<sock_type>::__start_worker() {
    printf("start worker\n");
    if (listen_fd >= 0) {
        if (listen(listen_fd, KSERVER_BACKLOG) < 0) {
            return -1;
        }
        comm_thread = std::thread{comm_thread_call<sock_type>, this};
    }
    printf("end worker\n");
    return 0;
}

// ---- TCP ----

template<>
int ListeningChannel<TCP>::init() {
    num_threads.store(0);

    if (config::tcp_worker_connections > 0) {
        listen_fd = create_tcp_listening(config::tcp_port);
        printf("TCP channel initialized\n");
        return listen_fd;
    }
    return 0;
}

template<>
void ListeningChannel<TCP>::shutdown() {
    if (config::tcp_worker_connections > 0) {
        ::shutdown(listen_fd, SHUT_RDWR);
        close(listen_fd);
    }
}

template<>
int ListeningChannel<TCP>::open_communication() {
    printf("TCP open_communication\n");
    return open_tcp_communication(listen_fd);
}

template<>
bool ListeningChannel<TCP>::is_max_threads() {
    return num_threads.load() >= (int)config::tcp_worker_connections;
}

template<>
int ListeningChannel<TCP>::start_worker() {
    return __start_worker();
}


// ---- WEBSOCK ----

template<>
int ListeningChannel<WEBSOCK>::init() {
    num_threads.store(0);

    if (config::websock_worker_connections > 0) {
        listen_fd = create_tcp_listening(config::websock_port);
        printf("Websocket channel initialized\n");
        return listen_fd;
    } else {
        return 0; // Nothing to be done
    }
}

template<>
void ListeningChannel<WEBSOCK>::shutdown() {
    if (config::websock_worker_connections > 0) {
        ::shutdown(listen_fd, SHUT_RDWR);
        close(listen_fd);
    }
}

template<>
int ListeningChannel<WEBSOCK>::open_communication() {
    return open_tcp_communication(listen_fd);
}

template<>
bool ListeningChannel<WEBSOCK>::is_max_threads() {
    return num_threads.load() >= (int)config::websock_worker_connections;
}

template<>
int ListeningChannel<WEBSOCK>::start_worker() {
    return __start_worker();
}

// ---- UNIX ----

int create_unix_listening(const char *unix_sock_path) {
    struct sockaddr_un local;

    int listen_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);

    if (listen_fd_ < 0) {
        return -1;
    }

    memset(&local, 0, sizeof(struct sockaddr_un));
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, unix_sock_path);
    unlink(local.sun_path);
    int len = strlen(local.sun_path) + sizeof(local.sun_family);

    if (bind(listen_fd_, (struct sockaddr *) &local, len) < 0) {
        close(listen_fd_);
        return -1;
    }

    return listen_fd_;
}

template<>
int ListeningChannel<UNIX>::init()
{
    num_threads.store(0);

    if (config::unixsock_worker_connections > 0) {
        listen_fd = create_unix_listening(config::unixsock_path);
        printf("Unix socket channel initialized\n");
        return listen_fd;
    }
    return 0;
}

template<>
void ListeningChannel<UNIX>::shutdown() {
    if (config::unixsock_worker_connections > 0) {
        ::shutdown(listen_fd, SHUT_RDWR);
        close(listen_fd);
    }
}

template<>
int ListeningChannel<UNIX>::open_communication() {
    struct sockaddr_un remote;
    uint32_t t = sizeof(remote);
    return accept(listen_fd, (struct sockaddr *)&remote, &t);
}

template<>
bool ListeningChannel<UNIX>::is_max_threads() {
    return num_threads.load() >= (int)config::unixsock_worker_connections;
}

template<>
int ListeningChannel<UNIX>::start_worker() {
    return __start_worker();
}

} // namespace kserver
