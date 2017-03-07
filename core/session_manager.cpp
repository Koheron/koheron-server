/// Implementation of session_manager.hpp
///
/// (c) Koheron

#include "session_manager.hpp"
#include "kserver_session.hpp"
#include "kserver.hpp"

#include <sys/socket.h>

#  include <thread>
#  include <mutex>

namespace kserver {

SessionManager::SessionManager(KServer& kserver_, DeviceManager& dev_manager_)
: kserver(kserver_),
  dev_manager(dev_manager_),
  session_pool(),
  reusable_ids(0)
{}

SessionManager::~SessionManager() {delete_all();}

unsigned int SessionManager::num_sess = 0;

bool SessionManager::is_reusable_id(SessID id)
{
    for (auto& reusable_id : reusable_ids)
        if (reusable_id == id)
            return true;

    return false;
}

bool SessionManager::is_current_id(SessID id)
{
    auto curr_ids = get_current_ids();

    for (auto& curr_id : curr_ids)
        if (curr_id == id)
            return true;

    return false;
}

std::vector<SessID> SessionManager::get_current_ids()
{
    std::vector<SessID> res(0);

    for (auto it = session_pool.begin(); it != session_pool.end(); ++it) {
        assert(!is_reusable_id(it->first));
        res.push_back(it->first);
    }

    return res;
}

void SessionManager::delete_session(SessID id)
{
    std::lock_guard<std::mutex> lock(mutex);

    int sess_fd;

    if (!is_current_id(id)) {
        return;
    }

    if (session_pool[id] != nullptr) {
        switch (session_pool[id]->kind) {
          case TCP:
            sess_fd = cast_to_session<TCP>(session_pool[id])->comm_fd;
            break;
          case UNIX:
            sess_fd = cast_to_session<UNIX>(session_pool[id])->comm_fd;
            break;
          case WEBSOCK:
            sess_fd = cast_to_session<WEBSOCK>(session_pool[id])->comm_fd;
            break;
          default: assert(false);
        }

        close(sess_fd);
    }

    session_pool.erase(id);
    reusable_ids.push_back(id);
    num_sess--;
}

void SessionManager::delete_all()
{
    assert(num_sess == session_pool.size());

    if (!session_pool.empty()) {
        auto ids = get_current_ids();

        for (auto& id : ids) {
            delete_session(id);
        }
    }

    assert(num_sess == 0);
}

} // namespace kserver

