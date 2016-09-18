/// Commands of the KServer device
///
/// (c) Koheron

#include "kserver.hpp"

#include <ctime>

#include "commands.hpp"
#include "kserver_session.hpp"
#include "session_manager.hpp"
#include "pubsub.tpp"

namespace kserver {

#define KS_DEV_WRITE_STR_LEN 1024

#define GET_SESSION kserver->session_manager.get_session(sess_id)
#define GET_CMD_LOG GET_SESSION.GetCmdLog()

#define KSERVER_STRUCT_ARGUMENTS(cmd_name)                          \
  template<>                                                        \
  template<>                                                        \
  struct KDevice<KServer, KSERVER>::Argument<KServer::cmd_name>

#define KSERVER_PARSE_ARG(cmd_name)                                 \
  template<>                                                        \
  template<>                                                        \
  int KDevice<KServer, KSERVER>::                                   \
          parse_arg<KServer::cmd_name>(Command& cmd,                \
                        KDevice<KServer, KSERVER>::                 \
                            Argument<KServer::cmd_name>& args,      \
                            SessID sess_id)

#define KSERVER_EXECUTE_OP(cmd_name)                                \
  template<>                                                        \
  template<>                                                        \
  int KDevice<KServer, KSERVER>::                                   \
      execute_op<KServer::cmd_name>                                 \
          (const Argument<KServer::cmd_name>& args, SessID sess_id)

/////////////////////////////////////
// GET_VERSION
// Send the server commit version

KSERVER_STRUCT_ARGUMENTS(GET_VERSION) {};
KSERVER_PARSE_ARG(GET_VERSION) {return 0;}

#define xstr(s) str(s)
#define str(s) #s

KSERVER_EXECUTE_OP(GET_VERSION)
{
    return GET_SESSION.send_cstr(xstr(SHA));
}

/////////////////////////////////////
// GET_CMDS
// Send the commands numbers

KSERVER_STRUCT_ARGUMENTS(GET_CMDS) {};
KSERVER_PARSE_ARG(GET_CMDS) {return 0;}

KSERVER_EXECUTE_OP(GET_CMDS)
{
    return GET_SESSION.send_cstr(DEVICES_JSON);
}

/////////////////////////////////////
// GET_STATS
// Get listeners statistics

KSERVER_STRUCT_ARGUMENTS(GET_STATS) {};
KSERVER_PARSE_ARG(GET_STATS) {return 0;}

template<int sock_type>
int send_listener_stats(SessID sess_id, KServer *kserver,
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
        kserver->syslog.print<SysLog::ERROR>(
                              "KServer::GET_STATS Format error\n");
        return -1;
    }

    if (ret >= KS_DEV_WRITE_STR_LEN) {
        kserver->syslog.print<SysLog::ERROR>(
                              "KServer::GET_STATS Buffer overflow\n");
        return -1;
    }

    if ((bytes_send = GET_SESSION.send_cstr(send_str)) < 0)
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
                    std::time(nullptr) - kserver->start_time);

    if (ret < 0) {
        kserver->syslog.print<SysLog::ERROR>(
                              "KServer::GET_STATS Format error\n");
        return -1;
    }

    if (ret >= KS_DEV_WRITE_STR_LEN) {
        kserver->syslog.print<SysLog::ERROR>(
                              "KServer::GET_STATS Buffer overflow\n");
        return -1;
    }

    if ((bytes = GET_SESSION.send_cstr(send_str)) < 0)
        return -1;

    bytes_send += bytes;

#if KSERVER_HAS_TCP
    if ((bytes = send_listener_stats<TCP>(sess_id, kserver,
                    &(kserver->tcp_listener))) < 0)
        return -1;

    bytes_send += bytes;
#endif
#if KSERVER_HAS_WEBSOCKET
    if ((bytes = send_listener_stats<WEBSOCK>(sess_id, kserver,
                    &(kserver->websock_listener))) < 0)
        return -1;

    bytes_send += bytes;
#endif
#if KSERVER_HAS_UNIX_SOCKET
    if ((bytes = send_listener_stats<UNIX>(sess_id, kserver,
                    &(kserver->unix_listener))) < 0)
        return -1;

    bytes_send += bytes;
#endif

    // Send EORS (End Of KServer Stats)
    if ((bytes = GET_SESSION.send_cstr("EOKS\n")) < 0)
        return -1;

    kserver->syslog.print_dbg("[S] [%u bytes]\n", bytes_send+bytes);

    return 0;
}

/////////////////////////////////////
// GET_DEV_STATUS
// Send the status of each device

KSERVER_STRUCT_ARGUMENTS(GET_DEV_STATUS) {};
KSERVER_PARSE_ARG(GET_DEV_STATUS) {return 0;}
KSERVER_EXECUTE_OP(GET_DEV_STATUS) {return 0;}

/////////////////////////////////////
// GET_RUNNING_SESSIONS
// Send the running sessions

KSERVER_STRUCT_ARGUMENTS(GET_RUNNING_SESSIONS) {};
KSERVER_PARSE_ARG(GET_RUNNING_SESSIONS) {return 0;}

#define SET_SESSION_PARAMS(sock_type)                                             \
    case sock_type:                                                               \
      sock_type_name = #sock_type;                                                \
      perms = static_cast<Session<sock_type>*>(&session)->get_permissions();      \
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
    const SessionPermissions* perms;

    const auto& ids = kserver->session_manager.get_current_ids();

    for (auto& id : ids) {
        SessionAbstract& session = kserver->session_manager.get_session(id);

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

        // Permissions
        const char *perms_str;

        if (perms->write && perms->read)
            perms_str = "WR";
        else if (perms->write && !perms->read)
            perms_str = "W";
        else if (!perms->write && perms->read)
            perms_str = "R";
        else
            perms_str = "";

        int ret = snprintf(send_str, KS_DEV_WRITE_STR_LEN,
                           "%u:%s:%s:%u:%u:%u:%li:%s\n",
                           id, sock_type_name,
                           ip, port, req_num, err_num,
                           std::time(nullptr) - start_time,
                           perms_str);

        if (ret < 0) {
            kserver->syslog.print<SysLog::ERROR>(
                            "KServer::GET_RUNNING_SESSIONS Format error\n");
            return -1;
        }

        if (ret >= KS_DEV_WRITE_STR_LEN) {
            kserver->syslog.print<SysLog::ERROR>(
                          "KServer::GET_RUNNING_SESSIONS Buffer overflow\n");
            return -1;
        }

        if ((bytes = GET_SESSION.send_cstr(send_str)) < 0)
            return -1;

        bytes_send += bytes;
    }

    // Send EORS (End Of Running Sessions)
    if ((bytes = GET_SESSION.send_cstr("EORS\n")) < 0)
        return -1;

    kserver->syslog.print_dbg("[S] [%u bytes]\n", bytes_send + bytes);

    return 0;
}

/////////////////////////////////////
// SUBSCRIBE_PUBSUB
// Subscribe to a pubsub channel

KSERVER_STRUCT_ARGUMENTS(SUBSCRIBE_PUBSUB)
{
    uint32_t channel;
};

KSERVER_PARSE_ARG(SUBSCRIBE_PUBSUB)
{
    if (required_buffer_size<uint32_t>() != cmd.payload_size) {
        kserver->syslog.print<SysLog::ERROR>("Invalid payload size\n");
        return -1;
    }

    args.channel = std::get<0>(cmd.payload.deserialize<uint32_t>());
    return 0;
}

KSERVER_EXECUTE_OP(SUBSCRIBE_PUBSUB)
{
    return kserver->pubsub.subscribe(args.channel, sess_id);
}

/////////////////////////////////////
// BROADCAST_PING
// Trigger broadcast emission on a given channel

KSERVER_STRUCT_ARGUMENTS(PUBSUB_PING) {};
KSERVER_PARSE_ARG(PUBSUB_PING) {return 0;}

KSERVER_EXECUTE_OP(PUBSUB_PING)
{
    kserver->pubsub.emit<PubSub::SERVER_CHANNEL, PubSub::PING>(static_cast<uint32_t>(sess_id));
    return 0;
}

////////////////////////////////////////////////

#define KSERVER_EXECUTE_CMD(cmd_name)                               \
  {                                                                 \
      Argument<KServer::cmd_name> args;                             \
                                                                    \
      if (parse_arg<KServer::cmd_name>(cmd, args, cmd.sess_id) < 0) \
          return -1;                                                \
                                                                    \
      err = execute_op<KServer::cmd_name>(args, cmd.sess_id);       \
      return err;                                                   \
  }

template<>
int KDevice<KServer, KSERVER>::execute(Command& cmd)
{
#if KSERVER_HAS_THREADS
    std::lock_guard<std::mutex> lock(static_cast<KServer*>(this)->ks_mutex);
#endif

    int err;

    switch (cmd.operation) {
      case KServer::GET_VERSION:
        KSERVER_EXECUTE_CMD(GET_VERSION)
      case KServer::GET_CMDS:
        KSERVER_EXECUTE_CMD(GET_CMDS)
      case KServer::GET_STATS:
        KSERVER_EXECUTE_CMD(GET_STATS)
      case KServer::GET_DEV_STATUS:
        KSERVER_EXECUTE_CMD(GET_DEV_STATUS)
      case KServer::GET_RUNNING_SESSIONS:
        KSERVER_EXECUTE_CMD(GET_RUNNING_SESSIONS)
      case KServer::SUBSCRIBE_PUBSUB:
        KSERVER_EXECUTE_CMD(SUBSCRIBE_PUBSUB)
      case KServer::PUBSUB_PING:
        KSERVER_EXECUTE_CMD(PUBSUB_PING)
      case KServer::kserver_op_num:
      default:
        kserver->syslog.print<SysLog::ERROR>(
                              "KServer::execute Unknown operation\n");
        return -1;
    }
}

} // namespace kserver
