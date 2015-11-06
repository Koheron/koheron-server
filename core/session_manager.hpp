/// @file session_manager.hpp
///
/// @brief Session manager
///
/// @author Thomas Vanderbruggen <thomas@koheron.com>
/// @date 26/12/2014
///
/// (c) Koheron 2014

#ifndef __SESSION_MANAGER_HPP__
#define __SESSION_MANAGER_HPP__

#include <map>
#include <vector>
#include <stack>

#include "kserver_defs.hpp"
#include "config.hpp"

#if KSERVER_HAS_THREADS
#  include <mutex>
#endif

namespace kserver {

class Session;
class DeviceManager;
class KServer;
class PeerInfo;

class SessionManager
{
  public:
    SessionManager(KServer& kserver_, DeviceManager& dev_manager_,
                   int perm_policy_);
    
    ~SessionManager();
    
    static unsigned int num_sess;
    
    size_t GetNumSess() const;
    
    Session* CreateSession(KServerConfig *config_, int comm_fd, 
                           int sock_type, PeerInfo peer_info);
    
    std::vector<SessID> GetCurrentIDs();
    
    bool IsAlive(SessID id) {return __is_current_id(id);}
    
    Session& GetSession(SessID id) const;
    
    void DeleteSession(SessID id);
    
    void DeleteAll();

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
    std::map<SessID, Session*> session_pool;
    std::vector<SessID> reusable_ids;
    
    void __apply_permissions(Session *last_created_session);
    void __reset_permissions(SessID id);
    void __print_reusable_ids();
    bool __is_reusable_id(SessID id);
    bool __is_current_id(SessID id);
    
#if KSERVER_HAS_THREADS
    std::mutex mutex;
#endif
};

} // namespace kserver

#endif //__SESSION_MANAGER_HPP__


