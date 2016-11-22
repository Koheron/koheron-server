/// Sessions manager
///
/// (c) Koheron

#ifndef __SESSION_MANAGER_HPP__
#define __SESSION_MANAGER_HPP__

#include <map>
#include <vector>
#include <stack>
#include <memory>

#include "kserver_defs.hpp"
#include "config.hpp"
#include "peer_info.hpp"

#if KSERVER_HAS_THREADS
#  include <mutex>
#endif

namespace kserver {

class SessionAbstract;
template<int sock_type> class Session;
class DeviceManager;
class KServer;

class SessionManager
{
  public:
    SessionManager(KServer& kserver_, DeviceManager& dev_manager_,
                   int perm_policy_);

    ~SessionManager();

    static unsigned int num_sess;

    size_t get_num_sess() const {return session_pool.size();}

    template<int sock_type>
    SessID create_session(const std::shared_ptr<KServerConfig>& config_,
                          int comm_fd);

    std::vector<SessID> get_current_ids();

    SessionAbstract& get_session(SessID id) const {return *session_pool.at(id);}

    void delete_session(SessID id);
    void delete_all();

    KServer& kserver;
    DeviceManager& dev_manager;

    // Permission policies

    int perm_policy;
    SessID fcfs_id; // ID of the FCFS session holding the write permission
    std::stack<SessID> lclf_lifo; // LIFO with the LCFS IDs to attribute

    enum write_permission_policy {
        /// None:
        /// It's the war ! Users can write all together ... Not advisable
        NONE,
        /// First Come First Served:
        /// The first connected user has write access to the devices.
        /// When the first user disconnects, then next one to connect
        /// obtain the write permission.
        FCFS,
        /// Last Come First Served:
        /// The last connected user has write access to the devices
        /// Mainly thought for development perspectives
        /// where several persons are working on the same server
        LCFS,
        // TODO Evolve towards a model where users are requesting for rights
        // on a given device.
        write_permission_policy_num
    };

  private:
    // Sessions pool
    std::map<SessID, std::unique_ptr<SessionAbstract>> session_pool;
    std::vector<SessID> reusable_ids;

    template<int sock_type>
    void apply_permissions(const std::unique_ptr<Session<sock_type>>& last_created_session);

    void reset_permissions(SessID id);
    void print_reusable_ids();
    bool is_reusable_id(SessID id);
    bool is_current_id(SessID id);

#if KSERVER_HAS_THREADS
    std::mutex mutex;
#endif
};

} // namespace kserver

#endif //__SESSION_MANAGER_HPP__
