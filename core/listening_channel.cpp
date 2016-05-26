/// Implementation and specializations of 
/// the class ListeningChannel in kserver.hpp
///
/// (c) Koheron

#include "kserver.hpp"

#if KSERVER_HAS_THREADS
#include <thread>
#endif

#include <atomic>

extern "C" {
  #include <sys/socket.h>   // socket definitions
  #include <sys/types.h>    // socket types      
  #include <arpa/inet.h>    // inet (3) functions
  #include <netinet/tcp.h>
#if KSERVER_HAS_UNIX_SOCKET
  #include <sys/un.h>
#endif
}

#include "peer_info.hpp"
#include "session_manager.hpp"
#include "session_manager.tpp"
#include "kserver_session.hpp"

namespace kserver {

int create_tcp_listening(unsigned int port, SysLog *syslog, 
                         const std::shared_ptr<KServerConfig>& config)
{
    int listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);

    if (listen_fd_ < 0) {
        syslog->print(SysLog::PANIC, "Can't open socket\n");
        return -1;
    }

    // To avoid binding error
    // See http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#bind
    int yes = 1;

    if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, 
                  &yes, sizeof(int))==-1) {
        syslog->print(SysLog::CRITICAL, "Cannot set SO_REUSEADDR\n");
    }

#if KSERVER_HAS_TCP_NODELAY
    if (config->tcp_nodelay) {
        int one = 1;

        if (setsockopt(listen_fd_, IPPROTO_TCP, TCP_NODELAY, 
                      (char *)&one, sizeof(one)) < 0) {
            // This is only considered critical since it is performance
            // related but this doesn't prevent to use the socket
            // so only log the error and keep going ...
            syslog->print(SysLog::CRITICAL, "Cannot set TCP_NODELAY\n");
        }
    }
#endif

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(port);

    // Assign name (address) to socket
    if (bind(listen_fd_, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        syslog->print(SysLog::PANIC, "Binding error\n");
        close(listen_fd_);
        return -1;
    }

    return listen_fd_;
}

int set_comm_sock_opts(int comm_fd, SysLog *syslog,
                       const std::shared_ptr<KServerConfig>& config)
{
    int sndbuf_len = sizeof(uint32_t) * KSERVER_SIG_LEN;

    if (setsockopt(comm_fd, SOL_SOCKET, SO_SNDBUF, 
                  &sndbuf_len, sizeof(sndbuf_len)) < 0) {
        syslog->print(SysLog::CRITICAL, "Cannot set socket send options\n");	
        close(comm_fd);	
        return -1;
    }

    int rcvbuf_len = KSERVER_READ_STR_LEN;

    if (setsockopt(comm_fd, SOL_SOCKET, SO_RCVBUF, 
                  &rcvbuf_len, sizeof(rcvbuf_len)) < 0) {
        syslog->print(SysLog::CRITICAL, "Cannot set socket receive options\n");
        close(comm_fd);		
        return -1;
    }

#if KSERVER_HAS_TCP_NODELAY
    if (config->tcp_nodelay) {
        int one = 1;

        if (setsockopt(comm_fd, IPPROTO_TCP, TCP_NODELAY, 
                       (char *)&one, sizeof(one)) < 0) {
            syslog->print(SysLog::CRITICAL, "Cannot set TCP_NODELAY\n");
            close(comm_fd);		
            return -1;
        }
    }
#endif

    return 0;
}

int open_tcp_communication(int listen_fd, SysLog *syslog,
                           const std::shared_ptr<KServerConfig>& config)
{
    int comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL);

    if (comm_fd < 0) {
        syslog->print(SysLog::CRITICAL, "Connection to client rejected\n");
        return -1;
    }

    if (set_comm_sock_opts(comm_fd, syslog, config) < 0)
        return -1;

    return comm_fd;
}

template<int sock_type>
void session_thread_call(int comm_fd, PeerInfo peer_info, 
                         ListeningChannel<sock_type> *listener)
{
    listener->inc_thread_num();
    listener->stats.opened_sessions_num++;
    listener->stats.total_sessions_num++;

    SessID sid = listener->kserver->session_manager. template CreateSession<sock_type>(
                            listener->kserver->config, comm_fd, peer_info);

    auto session = static_cast<Session<sock_type>*>(
                        &listener->kserver->session_manager.GetSession(sid));

    listener->kserver->syslog.print(SysLog::INFO, 
                "Start session id = %u. "
                "Client IP = %s, port = %u. Start time = %li\n", 
                sid, session->GetClientIP(), session->GetClientPort(), 
                session->GetStartTime());

    if (session->Run() < 0)
        listener->kserver->syslog.print(SysLog::ERROR, 
                                        "An error occured during session\n");

    listener->kserver->syslog.print(SysLog::INFO, 
                "Close session id = %u with #req = %u. #err = %u\n", 
                sid, session->RequestNum(), session->ErrorNum());

    listener->stats.total_requests_num += session->RequestNum();
    listener->kserver->session_manager.DeleteSession(sid); 

    listener->dec_thread_num();
    listener->stats.opened_sessions_num--;
}

template<int sock_type>
void comm_thread_call(ListeningChannel<sock_type> *listener)
{
    // Sending the signal kserver->exit_comm
    // is not enough for immediate exit of
    // the thread as it is very likely to be
    // blocked in the accept() function.
    //
    // Probably need to use non-blocking sockets and select() ...

    while (!listener->kserver->exit_comm.load()) {
        int comm_fd;
        PeerInfo peer_info;

        comm_fd = listener->open_communication();

        if (comm_fd < 0)
            continue;

        if (sock_type == TCP || sock_type == WEBSOCK)
            peer_info = PeerInfo(comm_fd);

#if KSERVER_HAS_THREADS
        if (listener->is_max_threads()) {
            listener->kserver->syslog.print(SysLog::INFO, 
                        "Maximum number of workers exceeded\n");
            continue;
        }

        std::thread sess_thread(session_thread_call<sock_type>, 
                                comm_fd, peer_info, listener);
        sess_thread.detach();        
#else
        session_thread_call<sock_type>(comm_fd, peer_info, listener);
#endif
    // /!\ Everything here will be executed 
    //     before the session thread is over
    }
}

template<int sock_type>
int ListeningChannel<sock_type>::__start_worker()
{
    if (listen_fd >= 0) {
        if (listen(listen_fd, KSERVER_BACKLOG) < 0) {
            kserver->syslog.print(SysLog::PANIC, "Listen %s error\n", 
                                  listen_channel_desc[sock_type].c_str());
            return -1;
        }

#if KSERVER_HAS_THREADS
        comm_thread = std::thread{comm_thread_call<sock_type>, this};
#else
        comm_thread_call<sock_type>(this);
#endif
    }

    return 0;
}

// ---- TCP ----

#if KSERVER_HAS_TCP

template<>
int ListeningChannel<TCP>::init()
{
    num_threads.store(0);

    if (kserver->config->tcp_worker_connections > 0) {
        listen_fd = create_tcp_listening(kserver->config->tcp_port,
                                           &kserver->syslog, kserver->config);  
        return listen_fd;      
    } else {
        return 0; // Nothing to be done
    }
}

template<>
void ListeningChannel<TCP>::shutdown()
{
    if (kserver->config->tcp_worker_connections > 0) {
        kserver->syslog.print(SysLog::INFO, "Closing TCP listener ...\n");
        close(listen_fd);
    }
}

template<>
int ListeningChannel<TCP>::open_communication()
{
    return open_tcp_communication(listen_fd, &kserver->syslog,
                                  kserver->config);
}

template<>
bool ListeningChannel<TCP>::is_max_threads()
{
    return (num_threads.load() + 1)
                 > (int)kserver->config->tcp_worker_connections;
}

template<>
int ListeningChannel<TCP>::start_worker()
{
    return __start_worker();
}

#endif // KSERVER_HAS_TCP

// ---- WEBSOCK ----

#if KSERVER_HAS_WEBSOCKET

template<>
int ListeningChannel<WEBSOCK>::init()
{
    num_threads.store(0);

    if (kserver->config->websock_worker_connections > 0) {
        listen_fd = create_tcp_listening(kserver->config->websock_port,
                                           &kserver->syslog, kserver->config);  
        return listen_fd;      
    } else {
        return 0; // Nothing to be done
    }
}

template<>
void ListeningChannel<WEBSOCK>::shutdown()
{
    if (kserver->config->websock_worker_connections > 0) {
        kserver->syslog.print(SysLog::INFO, "Closing WebSocket listener ...\n");
        close(listen_fd);
    }
}

template<>
int ListeningChannel<WEBSOCK>::open_communication()
{
    return open_tcp_communication(listen_fd, &kserver->syslog,
                                  kserver->config);
}

template<>
bool ListeningChannel<WEBSOCK>::is_max_threads()
{
    return (num_threads.load() + 1)
                 > (int)kserver->config->websock_worker_connections;
}

template<>
int ListeningChannel<WEBSOCK>::start_worker()
{
    return __start_worker();
}

#endif // KSERVER_HAS_WEBSOCKET

// ---- UNIX ----

#if KSERVER_HAS_UNIX_SOCKET

int create_unix_listening(const char *unix_sock_path, SysLog *syslog)
{
    struct sockaddr_un local;

    int listen_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);

    if (listen_fd_ < 0) {
        syslog->print(SysLog::PANIC, "Can't open Unix socket\n");
        return -1;
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, unix_sock_path);
    unlink(local.sun_path);
    int len = strlen(local.sun_path) + sizeof(local.sun_family);

    if (bind(listen_fd_, (struct sockaddr *) &local, len) < 0) {
        syslog->print(SysLog::PANIC, "Unix socket binding error\n");
        close(listen_fd_);
        return -1;
    }

    return listen_fd_;
}

template<>
int ListeningChannel<UNIX>::init()
{
    num_threads.store(0);

    if (kserver->config->unixsock_worker_connections > 0) {
        listen_fd = create_unix_listening(kserver->config->unixsock_path, 
                                          &kserver->syslog);  
        return listen_fd;      
    } else {
        return 0; // Nothing to be done
    }
}

template<>
void ListeningChannel<UNIX>::shutdown()
{
    if (kserver->config->unixsock_worker_connections > 0) {
        kserver->syslog.print(SysLog::INFO, "Closing Unix listener ...\n");
        close(listen_fd);
    }
}

template<>
int ListeningChannel<UNIX>::open_communication()
{
    struct sockaddr_un remote;
    uint32_t t = sizeof(remote);

    int comm_fd_unix = accept(listen_fd, (struct sockaddr *)&remote, &t);

    if (comm_fd_unix < 0) {
        kserver->syslog.print(SysLog::CRITICAL, 
                              "Connection to client rejected\n");
        return -1;
    }

    return comm_fd_unix;
}

template<>
bool ListeningChannel<UNIX>::is_max_threads()
{
    return (num_threads.load() + 1)
                 > (int)kserver->config->unixsock_worker_connections;
}

template<>
int ListeningChannel<UNIX>::start_worker()
{
    return __start_worker();
}

#endif // KSERVER_HAS_UNIX_SOCKET

} // namespace kserver
