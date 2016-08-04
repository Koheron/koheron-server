/// Implementation of session_manager.hpp
///
/// (c) Koheron

#include "session_manager.hpp"

#include "kserver_session.hpp"
#include "broadcast.tpp"

#include <sys/socket.h>

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

unsigned int SessionManager::num_sess = 0;

void SessionManager::print_reusable_ids()
{
    if (reusable_ids.size() == 0) {
        printf("reusable_ids = {}\n");
    } else {
        printf("reusable_ids = {%u", reusable_ids[0]);

        for (auto& reusable_id : reusable_ids)
            printf(", %u", reusable_id);
            
        printf("}\n");
    }
}

bool SessionManager::is_reusable_id(SessID id)
{
    for (auto& reusable_id : reusable_ids)
        if (reusable_id == id)
            return true;

    return false;
}

bool SessionManager::is_current_id(SessID id)
{
    auto curr_ids = GetCurrentIDs();

    for (auto& curr_id : curr_ids)
        if (curr_id == id)
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
                switch(session_pool[lclf_lifo.top()]->kind) {
#if KSERVER_HAS_TCP
                  case TCP:
                    cast_to_session<TCP>(session_pool[lclf_lifo.top()])
                                        ->permissions.write = true;
                    break;
#endif
#if KSERVER_HAS_UNIX_SOCKET
                  case UNIX:
                    cast_to_session<UNIX>(session_pool[lclf_lifo.top()])
                                        ->permissions.write = true;
                    break;
#endif
#if KSERVER_HAS_WEBSOCKET
                  case WEBSOCK:
                    cast_to_session<WEBSOCK>(session_pool[lclf_lifo.top()])
                                        ->permissions.write = true;
                    break;
#endif
                  default: assert(false);
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

    int sess_fd;

    if (!is_current_id(id)) {
        kserver.syslog.print(SysLog::INFO,
                             "Not allocated session ID: %u\n", id);
        return;
    }

    // Unsubscribe from any broadcast channel
    kserver.broadcast.unsubscribe(id);

    if (session_pool[id] != nullptr) {
        switch (session_pool[id]->kind) {
#if KSERVER_HAS_TCP
          case TCP:
            sess_fd = cast_to_session<TCP>(session_pool[id])->comm_fd;
            break;
#endif
#if KSERVER_HAS_UNIX_SOCKET
          case UNIX:
            sess_fd = cast_to_session<UNIX>(session_pool[id])->comm_fd;
            break;
#endif
#if KSERVER_HAS_WEBSOCKET
          case WEBSOCK:
            sess_fd = cast_to_session<WEBSOCK>(session_pool[id])->comm_fd;
            break;
#endif
          default: assert(false);
        }

        if (shutdown(sess_fd, SHUT_RDWR) < 0)
            kserver.syslog.print(SysLog::WARNING,
                         "Cannot shutdown socket for session ID: %u\n", id);
        close(sess_fd);
    }

    reset_permissions(id);
    session_pool.erase(id);
    reusable_ids.push_back(id);
    num_sess--;

    kserver.broadcast.emit<Broadcast::SERVER_CHANNEL, Broadcast::DEL_SESSION>();
}

void SessionManager::DeleteAll()
{
    kserver.syslog.print(SysLog::INFO, "Closing all active sessions ...\n");
    assert(num_sess == session_pool.size());

    if (!session_pool.empty()) {
        auto ids = GetCurrentIDs();
        
        for (auto& id : ids) {
            kserver.syslog.print(SysLog::INFO, "Delete session %u\n", id);
            DeleteSession(id);
        }
    }

    assert(num_sess == 0);
}

} // namespace kserver

