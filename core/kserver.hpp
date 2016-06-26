/// Server main class
///
/// (c) Koheron

#ifndef __KSERVER_HPP__
#define __KSERVER_HPP__

#include "kserver_defs.hpp"
#include "socket_interface_defs.hpp"

#if KSERVER_HAS_THREADS
#include <thread>
#include <mutex>
#endif

#include <array>
#include <vector>
#include <string>
#include <atomic>
#include <ctime>
#include <utility>

#include "kdevice.hpp"
#include "devices_manager.hpp"
#include "kserver_syslog.hpp"
#include "signal_handler.hpp"
#include "peer_info.hpp"
#include "session_manager.hpp"

namespace kserver {

template<int sock_type> class Session;

////////////////////////////////////////////////////////////////////////////
/////// Broadcast

class Broadcast
{
  public:
    Broadcast(SessionManager& session_manager_);

    // Session sid subscribes to a channel 
    int subscribe(uint32_t channel, SessID sid);

    void emit(uint32_t channel, uint32_t event);

    enum Channels {
        SERVER_CHANNEL,
        broadcast_channels_num
    };

    enum ServerChanEvents {
        PING,                   ///< For tests
        NEW_SESSION,            ///< A new session has been started
        server_chan_events_num
    };

  private:
    SessionManager& session_manager;

    template<uint32_t channel> using Subscribers = std::vector<SessID>;
    Subscribers<SERVER_CHANNEL> subscribers;

    // Event message structure
    // |      RESERVED     |      CHANNEL      |       EVENT       |   Arguments
    // |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | ...
    template<uint32_t channel, uint32_t event, typename... Tp>
    void emit_event(Tp... args);
};

// template<> auto emit_event<SERVER_CHANNEL, PING>();
// template<> auto emit_event<SERVER_CHANNEL, NEW_SESSION, sock_type>(int sock_type);

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
    if (listen_fd >= 0)
        comm_thread.detach();
}

template<int sock_type>
void ListeningChannel<sock_type>::join_worker()
{
    if (listen_fd >= 0)
        comm_thread.join();
}
#endif // KSERVER_HAS_THREADS

////////////////////////////////////////////////////////////////////////////
/////// KServer

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
    KServer(std::shared_ptr<kserver::KServerConfig> config_);
    ~KServer();
    
    int Run(void);

    /// Operations associated to the device
    enum Operation {
        GET_VERSION,          ///< Send th version of the server
        GET_CMDS,             ///< Send the commands numbers
        GET_STATS,            ///< Get KServer listeners statistics
        GET_DEV_STATUS,       ///< Send the devices status
        GET_RUNNING_SESSIONS, ///< Send the running sessions
        SUBSCRIBE_BROADCAST,  ///< Subscribe to a broadcast channel
        TRIGGER_BROADCAST,    ///< Trigger broadcast emission on a given channel
        kserver_op_num
    };
    
    std::shared_ptr<kserver::KServerConfig> config;
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

    Broadcast broadcast;
    
#if KSERVER_HAS_THREADS
    std::mutex ks_mutex;
#endif
    
  private:
    // Internal functions
    int start_listeners_workers();
    void detach_listeners_workers();
    void join_listeners_workers();    
    void close_listeners();

    template<int sock_type>
    void save_session_logs(Session<sock_type> *session, PeerInfo peer_info);
    
template<int sock_type> friend class ListeningChannel;
};

} // namespace kserver

#endif // __KSERVER_HPP__
