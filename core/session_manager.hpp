/// Sessions manager
///
/// (c) Koheron

#ifndef __SESSION_MANAGER_HPP__
#define __SESSION_MANAGER_HPP__

#include <map>
#include <vector>
#include <stack>

#include "kserver_defs.hpp"
#include "config.hpp"
#include "peer_info.hpp"
#include "kserver.hpp"

#if KSERVER_HAS_THREADS
#  include <mutex>
#endif

namespace kserver {

class SessionAbstract;
template<int sock_type> class Session;
class DeviceManager;

class SessionManager
{
  public:
    SessionManager(KServer& kserver_, DeviceManager& dev_manager_,
                   int perm_policy_);

    ~SessionManager();

    static unsigned int num_sess;

    size_t GetNumSess() const;

    template<int sock_type>
    Session<sock_type>* CreateSession(KServerConfig *config_, int comm_fd, 
                           PeerInfo peer_info);

    std::vector<SessID> GetCurrentIDs();

    SessionAbstract& GetSession(SessID id) const;

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
    std::map<SessID, SessionAbstract*> session_pool;
    std::vector<SessID> reusable_ids;

    template<int sock_type>
    void apply_permissions(Session<sock_type> *last_created_session);

    void __reset_permissions(SessID id);
    void __print_reusable_ids();
    bool __is_reusable_id(SessID id);
    bool __is_current_id(SessID id);

#if KSERVER_HAS_THREADS
    std::mutex mutex;
#endif
};

template<int sock_type>
Session<sock_type>* SessionManager::CreateSession(KServerConfig *config_, int comm_fd,
                                       PeerInfo peer_info)
{
#if KSERVER_HAS_THREADS
    std::lock_guard<std::mutex> lock(mutex);
#endif

    SessID new_id;
    
    // Choose a reusable ID if available else
    // create a new ID equal to the session number
    if (reusable_ids.size() == 0) {    
        new_id = num_sess;
    } else {
        new_id = reusable_ids.back();
        reusable_ids.pop_back();
    }

    auto session = new Session<sock_type>(config_, comm_fd, new_id, peer_info, (*this));
    assert(session != nullptr);
    
    apply_permissions(session);

    session_pool.insert(std::pair<SessID, SessionAbstract*>((SessID)new_id, 
                        static_cast<SessionAbstract*>(session)));
    num_sess++;
    return session;
}

template<int sock_type>
void SessionManager::apply_permissions(Session<sock_type> *last_created_session)
{
    switch (perm_policy) {
      case NONE:
        last_created_session->permissions.write = true;
        break;
      case FCFS:
        if (fcfs_id == -1) { // Write permission not attributed
            last_created_session->permissions.write = true;
            fcfs_id = last_created_session->GetID();
        } else {
            last_created_session->permissions.write = false;
        }
            
        break;
      case LCFS:
        // Set write permission of all current sessions to false
        if (!session_pool.empty()) {
            std::vector<SessID> ids = GetCurrentIDs();
            
            for (size_t i=0; i<ids.size(); i++)         
                static_cast<Session<sock_type>*>(session_pool[ids[i]])
                        ->permissions.write = false;
        }
        
        last_created_session->permissions.write = true;
        lclf_lifo.push(last_created_session->GetID());        
        break;
      default:
        kserver.syslog.print(SysLog::ERROR, "BUG: Invalid permission policy\n");
        last_created_session->permissions.write = DFLT_WRITE_PERM;
    }
}

SessionAbstract& SessionManager::GetSession(SessID id) const
{
    // This is not true anymore in a multithreaded environment:
    // If two sessions #0 and #1 are launch, and then #0 is closed
    // then num_sess is 1 but id #1 is still alive, violating the assertion
    //assert(id < num_sess);
    
    assert(session_pool.at(id) != nullptr);
    return *session_pool.at(id);
}

} // namespace kserver

#endif //__SESSION_MANAGER_HPP__


