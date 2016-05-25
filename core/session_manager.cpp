/// Implementation of session_manager.hpp
///
/// (c) Koheron

#include "session_manager.hpp"

#include "kserver_session.hpp"

#if KSERVER_HAS_THREADS
#  include <thread>
#  include <mutex>
#endif

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

void SessionManager::print_reusable_ids()
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

SessionAbstract& SessionManager::GetSession(SessID id) const
{
    assert(session_pool.at(id) != nullptr);
    return *session_pool.at(id);
}

bool SessionManager::is_reusable_id(SessID id)
{
    for (size_t i=0; i<reusable_ids.size(); i++)
        if (reusable_ids[i] == id)
            return true;

    return false;
}

bool SessionManager::is_current_id(SessID id)
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
        assert(!is_reusable_id(it->first));
        res.push_back(it->first);
    }

    return res;
}

void SessionManager::reset_permissions(SessID id)
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
            while (lclf_lifo.size() > 0 && !is_current_id(lclf_lifo.top()))
                lclf_lifo.pop();
            
            if (lclf_lifo.size() > 0) {
                SessionAbstract *session = session_pool[lclf_lifo.top()];

                switch(session->kind) {
                    case TCP:
                        static_cast<Session<TCP>*>(session)->permissions.write = true;
                        break;
                    case UNIX:
                        static_cast<Session<UNIX>*>(session)->permissions.write = true;
                        break;
                    case WEBSOCK:
                        static_cast<Session<WEBSOCK>*>(session)->permissions.write = true;
                        break;
                    default:
                        kserver.syslog.print(SysLog::ERROR, "Unknow socket type\n");
                }
            }
        }
    }
}

void SessionManager::DeleteSession(SessID id)
{
#if KSERVER_HAS_THREADS
    std::lock_guard<std::mutex> lock(mutex);
#endif

    if (!is_current_id(id)) {
        kserver.syslog.print(SysLog::INFO, 
                             "Not allocated session ID: %u\n", id);
        return;
    }

    if (session_pool[id] != NULL) {
        SessionAbstract *session = session_pool[id];

        switch (session->kind) {
            case TCP:
                close(static_cast<Session<TCP>*>(session)->comm_fd);
                delete static_cast<Session<TCP>*>(session);
                break;
            case UNIX:
                close(static_cast<Session<UNIX>*>(session)->comm_fd);
                delete static_cast<Session<UNIX>*>(session);
                break;
            case WEBSOCK:
                close(static_cast<Session<WEBSOCK>*>(session)->comm_fd);
                delete static_cast<Session<WEBSOCK>*>(session);
                break;
            default:
                kserver.syslog.print(SysLog::ERROR, "Unknow socket type\n");
        }
    }

    reset_permissions(id);

    session_pool.erase(id);
    reusable_ids.push_back(id);
    num_sess--;
}

void SessionManager::DeleteAll()
{
#if KSERVER_HAS_THREADS
    std::lock_guard<std::mutex> lock(mutex);
#endif

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

