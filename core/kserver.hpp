/// Server main class
///
/// (c) Koheron

#ifndef __KSERVER_HPP__
#define __KSERVER_HPP__

#include "kserver_defs.hpp"
#include "socket_interface_defs.hpp"

#include <thread>
#include <mutex>
#include <array>
#include <vector>
#include <string>
#include <atomic>
#include <ctime>
#include <utility>

#include "devices_manager.hpp"
#include "signal_handler.hpp"
#include "session_manager.hpp"

namespace kserver {

template<int sock_type> class Session;

/// Implementation in listening_channel.cpp
template<int sock_type>
class ListeningChannel
{
  public:
    ListeningChannel(KServer *kserver_)
    : listen_fd(-1)
    , is_ready(false)
    , kserver(kserver_)
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
    void join_worker();

    int open_communication();

    /// Listening socket ID
    int listen_fd;

    /// Number of sessions using the channel
    std::atomic<int> num_threads;

    /// True when ready to open sessions
    std::atomic<bool> is_ready;

    std::thread comm_thread; ///< Listening thread

    KServer *kserver;

  private:
    int __start_worker();
}; // ListeningChannel

template<int sock_type>
void ListeningChannel<sock_type>::join_worker() {
    if (listen_fd >= 0) {
        comm_thread.join();
    }
}

 ////////////////////////////////////////////////////////////////////////////
/////// KServer

/// Main class of the server. It initializes the
/// connections and start the sessions.
///
/// Derived from KDevice, therefore it is seen as
/// a device from the client stand point.

class KServer
{
  public:
    KServer();

    int run();

    /// Operations associated to the device
    enum Operation {
        GET_VERSION = 0,            ///< Send th version of the server
        GET_CMDS = 1,               ///< Send the commands numbers
        kserver_op_num
    };

    SignalHandler sig_handler;

    std::atomic<bool> exit_comm;
    std::atomic<bool> exit_all;

    // Listeners
    ListeningChannel<TCP> tcp_listener;
    ListeningChannel<WEBSOCK> websock_listener;
    ListeningChannel<UNIX> unix_listener;

    /// True when all listeners are ready
    bool is_ready();

    // Managers
    DeviceManager dev_manager;
    SessionManager session_manager;

    std::mutex ks_mutex;

    int execute(Command& cmd);
    template<int op> int execute_op(Command& cmd);

  private:
    // Internal functions
    int start_listeners_workers();
    void detach_listeners_workers();
    void join_listeners_workers();
    void close_listeners();

template<int sock_type> friend class ListeningChannel;
};

} // namespace kserver

#endif // __KSERVER_HPP__
