/// Implementation of session_manager.hpp
///
/// (c) Koheron

#include "session_manager.hpp"

#if KSERVER_HAS_THREADS
#  include <thread>
#  include <mutex>
#endif

#include "kserver_session.hpp"

namespace kserver {

SessionManager::SessionManager(KServer& kserver_, DeviceManager& dev_manager_, 
                               int perm_policy_)
: kserver(kserver_), 
  dev_manager(dev_manager_),
  perm_policy(perm_policy_),
  fcfs_id(-1),
  lclf_lifo(),
  session_pool(),
  reusable_ids(0)
{}

SessionManager::~SessionManager()
{
    DeleteAll();
}

size_t SessionManager::GetNumSess() const
{
    return session_pool.size();
}

unsigned int SessionManager::num_sess = 0;

Session* SessionManager::CreateSession(KServerConfig *config_, int comm_fd,
                                       int sock_type, PeerInfo peer_info)
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

    Session *session 
        = new Session(config_, comm_fd, new_id, sock_type, peer_info, (*this));
        
    assert(session != NULL);
    
    __apply_permissions(session);

    session_pool.insert(std::pair<SessID, Session*>((SessID)new_id, session));
    
    num_sess++;
    
    return session;
}

void SessionManager::__apply_permissions(Session *last_created_session)
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
                session_pool[ids[i]]->permissions.write = false;
        }
        
        last_created_session->permissions.write = true;
        lclf_lifo.push(last_created_session->GetID());        
        break;
      default:
        kserver.syslog.print(SysLog::ERROR, "BUG: Invalid permission policy\n");
        last_created_session->permissions.write = DFLT_WRITE_PERM;
    }
}

void SessionManager::__print_reusable_ids()
{
    if (reusable_ids.size() == 0) {
        printf("reusable_ids = {}\n");
    } else {
        printf("reusable_ids = {%u", reusable_ids[0]);
        
        for (size_t i=1; i<reusable_ids.size(); i++)
            printf(", %u", reusable_ids[i]);
            
        printf("}\n");
    }
}

bool SessionManager::__is_reusable_id(SessID id)
{
    for (size_t i=0; i<reusable_ids.size(); i++)
        if (reusable_ids[i] == id)
            return true;

    return false;
}

bool SessionManager::__is_current_id(SessID id)
{
    std::vector<SessID> curr_ids = GetCurrentIDs();
    
    for (size_t i=0; i<curr_ids.size(); i++)
        if (curr_ids[i] == id)
            return true;

    return false;
}

std::vector<SessID> SessionManager::GetCurrentIDs()
{
    std::vector<SessID> res(0);

    for (auto it = session_pool.begin(); it != session_pool.end(); ++it) {
        assert(!__is_reusable_id(it->first));
        res.push_back(it->first);
    }

    return res;
}

Session& SessionManager::GetSession(SessID id) const
{
    // This is not true anymore in a multithreaded environment:
    // If two sessions #0 and #1 are launch, and then #0 is closed
    // then num_sess is 1 but id #1 is still alive, violating the assertion
    //assert(id < num_sess);
    
    assert(session_pool.at(id) != NULL);
    
    return *session_pool.at(id);
}

void SessionManager::__reset_permissions(SessID id)
{
    switch (perm_policy) {
      case FCFS:
        // We reset the flag to indicate that the next 
        // opening session will have write permission.
        if(id == fcfs_id)
            fcfs_id = -1;
      case LCFS:
        // We remove from the LIFO the deleted session and
        // give back the writing rights to the previous
        // session holding them.
        if (id == lclf_lifo.top()) {
            lclf_lifo.pop();
            
            // Remove all the invalid IDs on the top of the LIFO
            while (lclf_lifo.size() > 0 && !__is_current_id(lclf_lifo.top()))
                lclf_lifo.pop();
            
            if (lclf_lifo.size() > 0)
                session_pool[lclf_lifo.top()]->permissions.write = true;
        }
    }
}

void SessionManager::DeleteSession(SessID id)
{
#if KSERVER_HAS_THREADS
    std::lock_guard<std::mutex> lock(mutex);
#endif

    if (!__is_current_id(id)) {
        kserver.syslog.print(SysLog::INFO, 
                             "Not allocated session ID: %u\n", id);
        return;
    }
    
    if (session_pool[id] != NULL) {
        close(session_pool[id]->comm_fd);
        delete session_pool[id];
    }
    
    __reset_permissions(id);

    session_pool.erase(id);
    reusable_ids.push_back(id);
    num_sess--;
}

void SessionManager::DeleteAll()
{
    assert(num_sess == session_pool.size());
    
    if (!session_pool.empty()) {
        std::vector<SessID> ids = GetCurrentIDs();
        
        for (size_t i=0; i<ids.size(); i++) {
            kserver.syslog.print(SysLog::INFO, "Delete session %u\n", ids[i]);            
            DeleteSession(ids[i]);
        }
    }
    
    assert(num_sess == 0);
}

} // namespace kserver

