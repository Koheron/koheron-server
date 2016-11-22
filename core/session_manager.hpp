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
    SessionManager(KServer& kserver_, DeviceManager& dev_manager_);

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

  private:
    // Sessions pool
    std::map<SessID, std::unique_ptr<SessionAbstract>> session_pool;
    std::vector<SessID> reusable_ids;

    void print_reusable_ids();
    bool is_reusable_id(SessID id);
    bool is_current_id(SessID id);

#if KSERVER_HAS_THREADS
    std::mutex mutex;
#endif
};

} // namespace kserver

#endif //__SESSION_MANAGER_HPP__
