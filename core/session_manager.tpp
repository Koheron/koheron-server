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
                                      int comm_fd)
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
                            config_, comm_fd, new_id, (*this));
    assert(session != nullptr);

    session_pool.insert(std::pair<SessID, std::unique_ptr<SessionAbstract>>(new_id,
                static_cast<std::unique_ptr<SessionAbstract>>(std::move(session))));
    num_sess++;

    return new_id;
}

} // namespace kserver
