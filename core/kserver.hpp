/// @file kserver.hpp
///
/// @brief KServer main class
///
/// @author Thomas Vanderbruggen <thomas@koheron.com>
/// @date 22/11/2014
///
/// (c) Koheron 2014

#ifndef __KSERVER_HPP__
#define __KSERVER_HPP__

#include "kserver_defs.hpp"

#if KSERVER_HAS_THREADS
#include <thread>
#include <mutex>
#endif

#include <array>
#include <string>
#include <atomic>
#include <ctime>

#include "kdevice.hpp"
#include "session_manager.hpp"
#include "devices_manager.hpp"
#include "kserver_syslog.hpp"
#include "signal_handler.hpp"

namespace kserver {

/// Listening channel types
enum SockType {
    NONE,
#if KSERVER_HAS_TCP
    TCP,
#endif
#if KSERVER_HAS_WEBSOCKET
    WEBSOCK,
#endif
#if KSERVER_HAS_UNIX_SOCKET
    UNIX,
#endif
    sock_type_num
};

/// Listening channel descriptions
const std::array< std::string, sock_type_num > 
listen_channel_desc = {{
    "NONE",
#if KSERVER_HAS_TCP
    "TCP",
#endif
#if KSERVER_HAS_WEBSOCKET
    "WebSocket",
#endif
#if KSERVER_HAS_UNIX_SOCKET
    "Unix socket"
#endif
}};

////////////////////////////////////////////////////////////////////////////
/////// ListeningChannel

template<int sock_type>
struct ListenerStats
{
    int opened_sessions_num = 0; ///< Number of currently opened sessions
    int total_sessions_num = 0;  ///< Total number of sessions
    int total_requests_num = 0;  ///< Total number of requests
};

/// Implementation in listening_channel.cpp
template<int sock_type>
class ListeningChannel
{
  public:
    ListeningChannel(KServer *kserver_)
    : listen_fd(-1),
      kserver(kserver_)
    {
        num_threads.store(-1);
    }

    int init();
    void shutdown();
    
    /// True if the maximum of threads set by the config is reached
    bool is_max_threads();
    
    inline void inc_thread_num() { num_threads++; }
    inline void dec_thread_num() { num_threads--; }
    
    int start_worker();
#if KSERVER_HAS_THREADS
    void detach_worker();
    void join_worker();
#endif
    
    int open_communication();
  
    /// Listening socket ID
    int listen_fd;
  
    /// Number of sessions using the channel
    std::atomic<int> num_threads;
    
#if KSERVER_HAS_THREADS
    std::thread comm_thread; ///< Listening thread
#endif

    KServer *kserver;
    ListenerStats<sock_type> stats;
        
  private:  
    int __start_worker();
}; // ListeningChannel

#if KSERVER_HAS_THREADS
template<int sock_type>
void ListeningChannel<sock_type>::detach_worker()
{
    if(listen_fd >= 0) {
        comm_thread.detach();
    }
}

template<int sock_type>
void ListeningChannel<sock_type>::join_worker()
{
    if(listen_fd >= 0) {
        comm_thread.join();
    }
}
#endif // KSERVER_HAS_THREADS

////////////////////////////////////////////////////////////////////////////
/////// KServer

class DeviceManager;

/// Main class of the server. It initializes the 
/// connections and start the sessions.
///
/// Derived from KDevice, therefore it is seen as 
/// a device from the client stand point.
class KServer : public KDevice<KServer, KSERVER> 
{
  public:
    const device_t kind = KSERVER;
    enum { __kind = KSERVER };

  public:
    KServer(KServerConfig *config_);
    ~KServer();
    
    /// @brief Run the server
    int Run(void);

    /// Operations associated to the device
    enum Operation {
        GET_ID,               ///< RESERVED
        GET_CMDS,             ///< Send the commands numbers
        GET_STATS,            ///< Get KServer listeners statistics
        GET_DEV_STATUS,       ///< Send the devices status
        GET_RUNNING_SESSIONS, ///< Send the running sessions
        KILL_SESSION,         ///< Kill a session (UNSTABLE)
        GET_SESSION_PERFS,    ///< Send the perfs of a session
        kserver_op_num
    };
    
    KServerConfig *config;
    SignalHandler sig_handler;
    
    std::atomic<bool> exit_comm;

    // Listeners    
#if KSERVER_HAS_TCP
    ListeningChannel<TCP> tcp_listener;
#endif
#if KSERVER_HAS_WEBSOCKET
    ListeningChannel<WEBSOCK> websock_listener;
#endif
#if KSERVER_HAS_UNIX_SOCKET
    ListeningChannel<UNIX> unix_listener;
#endif

    // Managers
    DeviceManager dev_manager;
    SessionManager session_manager;
	
    // Logs
    SysLog syslog;
    std::time_t start_time;
    
#if KSERVER_HAS_THREADS
    std::mutex ks_mutex;
#endif
    
  private:
    // Internal functions
    int start_listeners_workers();
    void detach_listeners_workers();
    void join_listeners_workers();    
    void close_listeners();   
    void save_session_logs(Session *session, PeerInfo peer_info);
    
template<int sock_type> friend class ListeningChannel;
}; // KServer

} // namespace kserver

#endif // __KSERVER_HPP__

