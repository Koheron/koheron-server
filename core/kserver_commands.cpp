/// Commands of the KServer device
///
/// (c) Koheron

#include "kserver.hpp"

#include <ctime>

#include "syslog.tpp"
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

/////////////////////////////////////
// GET_STATS
// Get listeners statistics

template<int sock_type>
int send_listener_stats(Command& cmd, KServer *kserver,
                        ListeningChannel<sock_type> *listener)
{
    char send_str[KS_DEV_WRITE_STR_LEN];
    unsigned int bytes_send = 0;

    int ret = snprintf(send_str, KS_DEV_WRITE_STR_LEN,
                    "%s:%d:%d:%d\n",
                    listen_channel_desc[sock_type].c_str(),
                    listener->stats.opened_sessions_num,
                    listener->stats.total_sessions_num,
                    listener->stats.total_requests_num);

    if (ret < 0) {
        kserver->syslog.print<ERROR>(
                              "KServer::GET_STATS Format error\n");
        return -1;
    }

    if (ret >= KS_DEV_WRITE_STR_LEN) {
        kserver->syslog.print<ERROR>(
                              "KServer::GET_STATS Buffer overflow\n");
        return -1;
    }

    if ((bytes_send = kserver->GET_SESSION.send<1, KServer::GET_STATS>(send_str)) < 0)
        return -1;

    return bytes_send;
}

KSERVER_EXECUTE_OP(GET_STATS)
{
    char send_str[KS_DEV_WRITE_STR_LEN];
    unsigned int bytes = 0;
    unsigned int bytes_send = 0;

    // Send start time
    int ret = snprintf(send_str, KS_DEV_WRITE_STR_LEN,
                    "%s:%lu\n", "UPTIME",
                    std::time(nullptr) - start_time);

    if (ret < 0) {
        syslog.print<ERROR>(
                      "KServer::GET_STATS Format error\n");
        return -1;
    }

    if (ret >= KS_DEV_WRITE_STR_LEN) {
        syslog.print<ERROR>(
                      "KServer::GET_STATS Buffer overflow\n");
        return -1;
    }

    if ((bytes = GET_SESSION.send<1, KServer::GET_STATS>(send_str)) < 0)
        return -1;

    bytes_send += bytes;

#if KSERVER_HAS_TCP
    if ((bytes = send_listener_stats<TCP>(cmd, this, &tcp_listener)) < 0)
        return -1;

    bytes_send += bytes;
#endif
#if KSERVER_HAS_WEBSOCKET
    if ((bytes = send_listener_stats<WEBSOCK>(cmd, this, &websock_listener)) < 0)
        return -1;

    bytes_send += bytes;
#endif
#if KSERVER_HAS_UNIX_SOCKET
    if ((bytes = send_listener_stats<UNIX>(cmd, this, &unix_listener)) < 0)
        return -1;

    bytes_send += bytes;
#endif

    // Send EORS (End Of KServer Stats)
    if ((bytes = GET_SESSION.send<1, KServer::GET_STATS>("EOKS\n")) < 0)
        return -1;

    syslog.print<DEBUG>("[S] [%u bytes]\n", bytes_send + bytes);
    return 0;
}

/////////////////////////////////////
// GET_DEV_STATUS
// Send the status of each device

// TODO: Remove

KSERVER_EXECUTE_OP(GET_DEV_STATUS) {return 0;}

/////////////////////////////////////
// GET_RUNNING_SESSIONS
// Send the running sessions

#define SET_SESSION_PARAMS(sock_type)                                             \
    case sock_type:                                                               \
      sock_type_name = #sock_type;                                                \
      ip = static_cast<Session<sock_type>*>(&session)->get_client_ip();           \
      port = static_cast<Session<sock_type>*>(&session)->get_client_port();       \
      req_num = static_cast<Session<sock_type>*>(&session)->request_num();        \
      err_num = static_cast<Session<sock_type>*>(&session)->error_num();          \
      start_time = static_cast<Session<sock_type>*>(&session)->get_start_time();  \
      break;

KSERVER_EXECUTE_OP(GET_RUNNING_SESSIONS)
{
    char send_str[KS_DEV_WRITE_STR_LEN];
    unsigned int bytes = 0;
    unsigned int bytes_send = 0;

    const auto& ids = session_manager.get_current_ids();

    for (auto& id : ids) {
        SessionAbstract& session = session_manager.get_session(id);

        const char *sock_type_name;
        const char *ip;
        uint32_t port, req_num, err_num;
        std::time_t start_time;

        switch (session.kind) {
#if KSERVER_HAS_TCP
          SET_SESSION_PARAMS(TCP)
#endif
#if KSERVER_HAS_WEBSOCKET
          SET_SESSION_PARAMS(WEBSOCK)
#endif
#if KSERVER_HAS_UNIX_SOCKET
          SET_SESSION_PARAMS(UNIX)
#endif
          default: assert(false);
        }

        int ret = snprintf(send_str, KS_DEV_WRITE_STR_LEN,
                           "%u:%s:%s:%u:%u:%u:%li\n",
                           id, sock_type_name,
                           ip, port, req_num, err_num,
                           std::time(nullptr) - start_time);

        if (ret < 0) {
            syslog.print<ERROR>(
                  "KServer::GET_RUNNING_SESSIONS Format error\n");
            return -1;
        }

        if (ret >= KS_DEV_WRITE_STR_LEN) {
            syslog.print<ERROR>(
                  "KServer::GET_RUNNING_SESSIONS Buffer overflow\n");
            return -1;
        }

        if ((bytes = GET_SESSION.send<1, KServer::GET_RUNNING_SESSIONS>(send_str)) < 0)
            return -1;

        bytes_send += bytes;
    }

    // Send EORS (End Of Running Sessions)
    if ((bytes = GET_SESSION.send<1, KServer::GET_RUNNING_SESSIONS>("EORS\n")) < 0)
        return -1;

    syslog.print<DEBUG>("[S] [%u bytes]\n", bytes_send + bytes);
    return 0;
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
      case KServer::GET_STATS:
        return execute_op<KServer::GET_STATS>(cmd);
      case KServer::GET_DEV_STATUS:
        return execute_op<KServer::GET_DEV_STATUS>(cmd);
      case KServer::GET_RUNNING_SESSIONS:
        return execute_op<KServer::GET_RUNNING_SESSIONS>(cmd);
      case KServer::kserver_op_num:
      default:
        syslog.print<ERROR>("KServer::execute unknown operation\n");
        return -1;
    }
}

} // namespace kserver
