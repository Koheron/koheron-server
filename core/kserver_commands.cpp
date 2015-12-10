/// @file kserver_commands.cpp
///
/// @brief Commands of the KServer device
///
/// @author Thomas Vanderbruggen <thomas@koheron.com>
/// @date 25/08/2015
///
/// (c) Koheron 2014-2015

#include "kserver.hpp"

#include <ctime>

#include "commands.hpp"
#include "kserver_session.hpp"

namespace kserver {

#define KS_DEV_WRITE_STR_LEN 1024

#define GET_SESSION kserver->session_manager.GetSession(sess_id)
#define GET_CMD_LOG GET_SESSION.GetCmdLog()

#define KSERVER_STRUCT_ARGUMENTS(cmd_name)                          \
  template<>                                                        \
  template<>                                                        \
  struct KDevice<KServer, KSERVER>::Argument<KServer::cmd_name>
  
#define KSERVER_PARSE_ARG(cmd_name)                                 \
  template<>                                                        \
  template<>                                                        \
  int KDevice<KServer, KSERVER>::                                   \
          parse_arg<KServer::cmd_name>(const Command& cmd,          \
                        KDevice<KServer, KSERVER>::                 \
                            Argument<KServer::cmd_name>& args)      

#define KSERVER_EXECUTE_OP(cmd_name)                                \
  template<>                                                        \
  template<>                                                        \
  int KDevice<KServer, KSERVER>::                                   \
      execute_op<KServer::cmd_name>                                 \
          (const Argument<KServer::cmd_name>& args, SessID sess_id)

/////////////////////////////////////
// GET_ID
// Send the ID of the board

KSERVER_STRUCT_ARGUMENTS(GET_ID)
{
    // No arguments
};

KSERVER_PARSE_ARG(GET_ID)
{
    return 0;
}

KSERVER_EXECUTE_OP(GET_ID)
{
    // TODO
    return 0;
}

/////////////////////////////////////
// GET_CMDS
// Send the commands numbers

KSERVER_STRUCT_ARGUMENTS(GET_CMDS)
{
    // No arguments
};

KSERVER_PARSE_ARG(GET_CMDS)
{
    return 0;
}

KSERVER_EXECUTE_OP(GET_CMDS)
{
    char cmds_str[KS_DEV_WRITE_STR_LEN];
    unsigned int bytes = 0;
    unsigned int bytes_send = 0;

    // Send MAX_OP_NUM
    int ret = snprintf(cmds_str, KS_DEV_WRITE_STR_LEN,
                        "%u\n", MAX_OP_NUM );

    if(ret < 0) {
        kserver->syslog.print(SysLog::ERROR, 
                              "KServer::GET_CMDS Format error\n");
        return -1;
    }

    if(ret >= KS_DEV_WRITE_STR_LEN) {
        kserver->syslog.print(SysLog::ERROR, 
                              "KServer::GET_CMDS Buffer overflow\n");
        return -1;
    }

    if((bytes = GET_SESSION.SendCstr(cmds_str)) < 0) {
        return -1;
    }

    bytes_send += bytes;
			
    // Send devices and operations
    // dev#:dev_name:op1:op2:op3:...:opn
    for(unsigned int i=0; i<device_num; i++) {
        ret = snprintf(cmds_str, KS_DEV_WRITE_STR_LEN,
                        "%u:%s", i, 
                        (device_desc[i][0]).c_str() );

        if(ret < 0) {
            kserver->syslog.print(SysLog::ERROR, 
                                  "KServer::GET_CMDS Format error\n");
            return -1;
        }

        if(ret >= KS_DEV_WRITE_STR_LEN) {
            kserver->syslog.print(SysLog::ERROR, 
                                  "KServer::GET_CMDS Buffer overflow\n");
            return -1;
        }

        for(unsigned int j=0; j<MAX_OP_NUM; j++) {
            strcat(cmds_str, ":");
            strcat(cmds_str, (device_desc[i][j+1]).c_str());
        }
				
        strcat(cmds_str, "\n");
				
        if((bytes = GET_SESSION.SendCstr(cmds_str)) < 0) {
            return -1;
        }

        bytes_send += bytes;
    }

    // Send EOC (End Of Commands)
    if((bytes = GET_SESSION.SendCstr("EOC\n")) < 0) {
       return -1;
    }
			
    kserver->syslog.print(SysLog::DEBUG, "[S] [%u bytes]\n", bytes_send+bytes);

    return 0;
}

/////////////////////////////////////
// GET_STATS
// Get KServer listeners statistics

KSERVER_STRUCT_ARGUMENTS(GET_STATS)
{
    // No arguments
};

KSERVER_PARSE_ARG(GET_STATS)
{
    return 0;
}

template<int sock_type>
int __send_listener_stats(SessID sess_id, KServer *kserver, 
                          ListeningChannel<sock_type> *listener)
{
    char send_str[KS_DEV_WRITE_STR_LEN];
    unsigned int bytes_send = 0;

    // sock_type:opened_sessions_num:total_sessions_num:total_requests_num
    int ret = snprintf(send_str, KS_DEV_WRITE_STR_LEN,
                    "%s:%d:%d:%d\n", 
                    listen_channel_desc[sock_type].c_str(), 
                    listener->stats.opened_sessions_num,
                    listener->stats.total_sessions_num,
                    listener->stats.total_requests_num);

    if(ret < 0) {
        kserver->syslog.print(SysLog::ERROR, 
                              "KServer::GET_STATS Format error\n");
        return -1;
    }

    if(ret >= KS_DEV_WRITE_STR_LEN) {
        kserver->syslog.print(SysLog::ERROR, 
                              "KServer::GET_STATS Buffer overflow\n");
        return -1;
    }

    if((bytes_send = GET_SESSION.SendCstr(send_str)) < 0)
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

    if(ret < 0) {
        kserver->syslog.print(SysLog::ERROR, 
                              "KServer::GET_STATS Format error\n");
        return -1;
    }

    if(ret >= KS_DEV_WRITE_STR_LEN) {
        kserver->syslog.print(SysLog::ERROR, 
                              "KServer::GET_STATS Buffer overflow\n");
        return -1;
    }

    if((bytes = GET_SESSION.SendCstr(send_str)) < 0)
        return -1;
        
    bytes_send += bytes;
    
#if KSERVER_HAS_TCP
    if((bytes = __send_listener_stats<TCP>(sess_id, kserver, 
                    &(kserver->tcp_listener))) < 0) {
        return -1;
    }
    
    bytes_send += bytes;
#endif
#if KSERVER_HAS_WEBSOCKET
    if((bytes = __send_listener_stats<WEBSOCK>(sess_id, kserver, 
                    &(kserver->websock_listener))) < 0) {
        return -1;
    }
    
    bytes_send += bytes;
#endif
#if KSERVER_HAS_UNIX_SOCKET
    if((bytes = __send_listener_stats<UNIX>(sess_id, kserver, 
                    &(kserver->unix_listener))) < 0) {
        return -1;
    }
    
    bytes_send += bytes;
#endif

    // Send EORS (End Of KServer Stats)
    if((bytes = GET_SESSION.SendCstr("EOKS\n")) < 0) {
        return -1;
    }

    kserver->syslog.print(SysLog::DEBUG, "[S] [%u bytes]\n", bytes_send+bytes);

    return 0;
}

/////////////////////////////////////
// GET_DEV_STATUS
// Send the status of each device

KSERVER_STRUCT_ARGUMENTS(GET_DEV_STATUS)
{
    // No arguments
};

KSERVER_PARSE_ARG(GET_DEV_STATUS)
{
    return 0;
}

KSERVER_EXECUTE_OP(GET_DEV_STATUS)
{
    char send_str[KS_DEV_WRITE_STR_LEN];
    unsigned int bytes = 0;
    unsigned int bytes_send = 0;

    // Send dev#:dev_name:status
    for(unsigned int i=KSERVER; i<device_num; i++) {
        int ret = snprintf(send_str, KS_DEV_WRITE_STR_LEN,
                    "%u:%s:%s\n", i, (device_desc[i][0]).c_str(), 
                    KS_dev_status_desc[ 
                        kserver->dev_manager.GetStatus((device_t)i) 
                    ].c_str());

        if(ret < 0) {
            kserver->syslog.print(SysLog::ERROR, 
                                  "KServer::GET_DEV_STATUS Format error\n");
            return -1;
        }

        if(ret >= KS_DEV_WRITE_STR_LEN) {
            kserver->syslog.print(SysLog::ERROR, 
                                  "KServer::GET_DEV_STATUS Buffer overflow\n");
            return -1;
        }

        if((bytes = GET_SESSION.SendCstr(send_str)) < 0) {
            return -1;
        }

        bytes_send += bytes;
    }

    // Send EODS (End Of Device Status)
    if((bytes = GET_SESSION.SendCstr("EODS\n")) < 0) {
        return -1;
    }

    kserver->syslog.print(SysLog::DEBUG, 
                          "[S] [%u bytes]\n", bytes_send + bytes);

    return 0;
}

/////////////////////////////////////
// GET_RUNNING_SESSIONS
// Send the running sessions

KSERVER_STRUCT_ARGUMENTS(GET_RUNNING_SESSIONS)
{
    // No arguments
};

KSERVER_PARSE_ARG(GET_RUNNING_SESSIONS)
{
    return 0;
}

KSERVER_EXECUTE_OP(GET_RUNNING_SESSIONS)
{
    char send_str[KS_DEV_WRITE_STR_LEN];
    unsigned int bytes = 0;
    unsigned int bytes_send = 0;

    // Send 
    // sess_id:
    // connection_type:
    // client_ip:
    // client_port:
    // request_num:
    // error_num:
    // start_time:
    // permissions
    
    std::vector<SessID> ids = kserver->session_manager.GetCurrentIDs();
    
    for(unsigned int i=0; i<ids.size(); i++) {
        Session& session = kserver->session_manager.GetSession(ids[i]);

        // Connection type
        const char *sock_type_name;

        switch(session.GetSockType()) {
#if KSERVER_HAS_TCP
          case TCP:
            sock_type_name = "TCP";
            break;
#endif
#if KSERVER_HAS_WEBSOCKET
          case WEBSOCK:
            sock_type_name = "WEBSOCK";
            break;
#endif
#if KSERVER_HAS_UNIX_SOCKET
          case UNIX:
            sock_type_name = "UNIX";
            break;
#endif
          default:
            kserver->syslog.print(SysLog::ERROR, 
                 "KServer::GET_RUNNING_SESSIONS Invalid connection type\n");
            return -1;
        }
        
        // Permissions
        const SessionPermissions* perms = session.GetPermissions();
        const char *perms_str;
        
        if(perms->write && perms->read)
            perms_str = "WR";
        else if(perms->write && !perms->read)
            perms_str = "W";
        else if(!perms->write && perms->read)
            perms_str = "R";
        else
            perms_str = "";

        int ret = snprintf(send_str, KS_DEV_WRITE_STR_LEN,
                           "%u:%s:%s:%u:%u:%u:%li:%s\n", 
                           ids[i], sock_type_name, 
                           session.GetClientIP(), session.GetClientPort(),
                           session.RequestNum(), session.ErrorNum(),
                           std::time(nullptr) - session.GetStartTime(),
                           perms_str);

        if(ret < 0) {
            kserver->syslog.print(SysLog::ERROR, 
                            "KServer::GET_RUNNING_SESSIONS Format error\n");
            return -1;
        }

        if(ret >= KS_DEV_WRITE_STR_LEN) {
            kserver->syslog.print(SysLog::ERROR, 
                          "KServer::GET_RUNNING_SESSIONS Buffer overflow\n");
            return -1;
        }

        if((bytes = GET_SESSION.SendCstr(send_str)) < 0) {
            return -1;
        }

        bytes_send += bytes;
    }

    // Send EORS (End Of Running Sessions)
    if((bytes = GET_SESSION.SendCstr("EORS\n")) < 0) {
        return -1;
    }

    kserver->syslog.print(SysLog::DEBUG, "[S] [%u bytes]\n", bytes_send+bytes);

    return 0;
}

/////////////////////////////////////
// KILL_SESSION
// Kill a session

KSERVER_STRUCT_ARGUMENTS(KILL_SESSION)
{
    SessID sid; ///< ID of the session to kill
};

KSERVER_PARSE_ARG(KILL_SESSION)
{
    char tmp_str[2*KSERVER_READ_STR_LEN];

    uint32_t i = 0;
    uint32_t cnt = 0;
    uint32_t param_num = 0;

    while(1) {
        if(cmd.buffer[i]=='\0') {
            break;
        }
        else if(cmd.buffer[i]=='|') {
            tmp_str[cnt] = '\0';

            if(param_num == 0)
                args.sid = static_cast<SessID>(CSTRING_TO_UINT(tmp_str));

            param_num++;
            cnt = 0;
            i++;
        } else {
            tmp_str[cnt] = cmd.buffer[i];
            i++;
            cnt++;
        }
    }

    if(param_num != 1) {
        kserver->syslog.print(SysLog::ERROR, "Invalid number of parameters\n");
        return -1;
    }

    return 0;
}

KSERVER_EXECUTE_OP(KILL_SESSION)
{
    if(args.sid == sess_id) {
        kserver->syslog.print(SysLog::ERROR, "Session can't kill itself\n");
        return -1;
    }
    
    std::vector<SessID> ids = kserver->session_manager.GetCurrentIDs();
    
    for(unsigned int i=0; i<ids.size(); i++) {        
        if(ids[i] == args.sid) {
            kserver->session_manager.DeleteSession(args.sid);
            return 0;
        }
    }
    
    kserver->syslog.print(SysLog::ERROR, "Invalid ID: %u\n", args.sid);
    return -1;
}

/////////////////////////////////////
// GET_SESSION_PERFS
// Send the perfs of a session

KSERVER_STRUCT_ARGUMENTS(GET_SESSION_PERFS)
{
    SessID sid; ///< ID of the session who perfs are wanted
};

KSERVER_PARSE_ARG(GET_SESSION_PERFS)
{
    char tmp_str[2*KSERVER_READ_STR_LEN];

    uint32_t i = 0;
    uint32_t cnt = 0;
    uint32_t param_num = 0;

    while(1) {
        if(cmd.buffer[i]=='\0') {
            break;
        }
        else if(cmd.buffer[i]=='|') {
            tmp_str[cnt] = '\0';

            if(param_num == 0)
                args.sid = static_cast<SessID>(CSTRING_TO_UINT(tmp_str));

            param_num++;
            cnt = 0;
            i++;
        } else {
            tmp_str[cnt] = cmd.buffer[i];
            i++;
            cnt++;
        }
    }

    if(param_num != 1) {
        kserver->syslog.print(SysLog::ERROR, "Invalid number of parameters\n");
        return -1;
    }

    return 0;
}

KSERVER_EXECUTE_OP(GET_SESSION_PERFS)
{
    char send_str[KS_DEV_WRITE_STR_LEN];
    unsigned int bytes = 0;
    unsigned int bytes_send = 0;

    std::vector<SessID> ids = kserver->session_manager.GetCurrentIDs();
    
    for(unsigned int i=0; i<ids.size(); i++) {        
        if(ids[i] == args.sid) {
            const PerfMonitor *perf 
                = kserver->session_manager.GetSession(args.sid).GetPerf();
                
            // Send:
            // timing_pt_name:mean_duration:min_duration:max_duration
            for(int j=0; j<timing_points_num; j++) {
                timing_point_t time_pt = static_cast<timing_point_t>(j);
            
                int ret = snprintf(send_str, KS_DEV_WRITE_STR_LEN,
                                "%s:%f:%d:%d\n", 
                                timing_points_desc[time_pt].c_str(),
                                perf->get_mean_duration(time_pt),
                                perf->get_min_duration(time_pt),
                                perf->get_max_duration(time_pt));

                if(ret < 0) {
                    kserver->syslog.print(SysLog::ERROR, 
                        "KServer::GET_SESSION_PERFS Format error\n");
                    return -1;
                }

                if(ret >= KS_DEV_WRITE_STR_LEN) {
                    kserver->syslog.print(SysLog::ERROR, 
                        "KServer::GET_SESSION_PERFS Buffer overflow\n");
                    return -1;
                }

                if((bytes = GET_SESSION.SendCstr(send_str)) < 0)
                    return -1;

                bytes_send += bytes;
            }            
                
            // Send EOSP (End Of Session Perf)
            if((bytes = GET_SESSION.SendCstr("EOSP\n")) < 0) {
                return -1;
            }

            kserver->syslog.print(SysLog::DEBUG, 
                                  "[S] [%u bytes]\n", bytes_send+bytes);
            return 0;
        }
    }
    
    kserver->syslog.print(SysLog::ERROR, 
                    "KServer::GET_SESSION_PERFS Invalid ID: %u\n", args.sid);
    return -1;
}

////////////////////////////////////////////////

#define KSERVER_EXECUTE_CMD(cmd_name)                               \
  {                                                                 \
      Argument<KServer::cmd_name> args;                             \
                                                                    \
      if(parse_arg<KServer::cmd_name>(cmd, args) < 0) {             \
          return -1;                                                \
      }                                                             \
                                                                    \
      err = execute_op<KServer::cmd_name>(args, cmd.sess_id);       \
      return err;                                                   \
  }

template<>
int KDevice<KServer, KSERVER>::execute(const Command& cmd)
{   
#if KSERVER_HAS_THREADS
    std::lock_guard<std::mutex> lock(static_cast<KServer*>(this)->ks_mutex);
#endif

    int err;
    
    switch(cmd.operation) {
      case KServer::GET_ID:
        KSERVER_EXECUTE_CMD(GET_ID)
      case KServer::GET_CMDS:
        KSERVER_EXECUTE_CMD(GET_CMDS)
      case KServer::GET_STATS:
        KSERVER_EXECUTE_CMD(GET_STATS)
      case KServer::GET_DEV_STATUS:
        KSERVER_EXECUTE_CMD(GET_DEV_STATUS)
      case KServer::GET_RUNNING_SESSIONS:
        KSERVER_EXECUTE_CMD(GET_RUNNING_SESSIONS)
      case KServer::KILL_SESSION:
        KSERVER_EXECUTE_CMD(KILL_SESSION)
      case KServer::GET_SESSION_PERFS:
        KSERVER_EXECUTE_CMD(GET_SESSION_PERFS)
      case KServer::kserver_op_num:
      default:
        kserver->syslog.print(SysLog::ERROR,
                              "KServer::execute Unknown operation\n");
        return -1;
    }
}

template<>
bool KDevice<KServer, KSERVER>::is_failed(void) 
{
    return 0; // Never fail
}

} // namespace kserver
