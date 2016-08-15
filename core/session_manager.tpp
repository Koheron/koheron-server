/// Sessions manager template implementation
///
/// (c) Koheron

#include "session_manager.hpp"

#include "kserver.hpp"
#include "kserver_session.hpp"
#include "pubsub.tpp"

namespace kserver {

template<int sock_type>
SessID SessionManager::create_session(const std::shared_ptr<KServerConfig>& config_,
                                      int comm_fd, PeerInfo peer_info)
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

    auto session = std::make_unique<Session<sock_type>>(
                            config_, comm_fd, new_id, peer_info, (*this));
    assert(session != nullptr);
    apply_permissions(session);

    session_pool.insert(std::pair<SessID, std::unique_ptr<SessionAbstract>>(new_id,
                static_cast<std::unique_ptr<SessionAbstract>>(std::move(session))));
    num_sess++;

    // Publish new session creation
    kserver.pubsub.emit<PubSub::SERVER_CHANNEL, PubSub::NEW_SESSION>(static_cast<uint32_t>(sock_type),
                                                                     static_cast<uint32_t>(new_id));

    return new_id;
}

template<int sock_type>
void SessionManager::apply_permissions(
    const std::unique_ptr<Session<sock_type>>& last_created_session)
{
    switch (perm_policy) {
      case NONE:
        last_created_session->permissions.write = true;
        break;
      case FCFS:
        if (fcfs_id == -1) { // Write permission not attributed
            last_created_session->permissions.write = true;
            fcfs_id = last_created_session->get_id();
        } else {
            last_created_session->permissions.write = false;
        }

        break;
      case LCFS:
        // Set write permission of all current sessions to false
        if (!session_pool.empty()) {
            const auto& ids = get_current_ids();
            
            for (auto& id : ids)
                cast_to_session<sock_type>(session_pool[id])
                                            ->permissions.write = false;
        }

        last_created_session->permissions.write = true;
        lclf_lifo.push(last_created_session->get_id());
        break;
      default:
        kserver.syslog.print(SysLog::ERROR, "BUG: Invalid permission policy\n");
        last_created_session->permissions.write = DFLT_WRITE_PERM;
    }
}

} // namespace kserver
