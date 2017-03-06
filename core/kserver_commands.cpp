/// Commands of the KServer device
///
/// (c) Koheron

#include "kserver.hpp"
#include "kserver_session.hpp"
#include <ctime>
#include <devices_json.hpp>

namespace kserver {

#define KS_DEV_WRITE_STR_LEN 1024

#define GET_SESSION session_manager.get_session(cmd.sess_id)
#define GET_CMD_LOG GET_SESSION.GetCmdLog()

#define KSERVER_EXECUTE_OP(cmd_name)                       \
  template<> int KServer::execute_op<KServer::cmd_name>(Command& cmd)

/////////////////////////////////////
// GET_VERSION
// Send the server commit version

#define xstr(s) str(s)
#define str(s) #s

KSERVER_EXECUTE_OP(GET_VERSION)
{
    return GET_SESSION.send<1, KServer::GET_VERSION>(xstr(KOHERON_SERVER_VERSION));
}

/////////////////////////////////////
// GET_CMDS
// Send the commands numbers

KSERVER_EXECUTE_OP(GET_CMDS)
{
    return GET_SESSION.send<1, KServer::GET_CMDS>(build_devices_json());
}

////////////////////////////////////////////////

int KServer::execute(Command& cmd)
{
    std::lock_guard<std::mutex> lock(static_cast<KServer*>(this)->ks_mutex);

    switch (cmd.operation) {
      case KServer::GET_VERSION:
        return execute_op<KServer::GET_VERSION>(cmd);
      case KServer::GET_CMDS:
        return execute_op<KServer::GET_CMDS>(cmd);
      default:
        return -1;
    }
}

} // namespace kserver
