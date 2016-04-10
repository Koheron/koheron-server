/// Implementation of kserver_session.hpp
///
/// (c) Koheron

#include "kserver_session.hpp"
#include "websocket.hpp"

namespace kserver {

Session::Session(KServerConfig *config_, int comm_fd_,
                 SessID id_, int sock_type_, PeerInfo peer_info_,
                 SessionManager& session_manager_)
: config(config_)
, comm_fd(comm_fd_)
, id(id_)
, syslog_ptr(&session_manager_.kserver.syslog)
, sock_type(sock_type_)
, peer_info(peer_info_)
, session_manager(session_manager_)
, permissions()
, requests_num(0)
, errors_num(0)
, start_time(0)
{
    assert(sock_type < sock_type_num);

    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP: {
        socket = new TCPSocketInterface(config_, &session_manager.kserver, 
                                        comm_fd, id_);
        break;
      }
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX: {
        socket = new UnixSocketInterface(config_, &session_manager.kserver, 
                                         comm_fd, id_);
        break;
      }
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK: {
        socket = new WebSocketInterface(config_, &session_manager.kserver, 
                                        comm_fd, id_);
        break;
      }
#endif
    }
}

Session::~Session()
{
    if(socket == nullptr)
        return;

    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP:
        delete TCPSOCKET;
        break;
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX:
        delete UNIXSOCKET;
        break;
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK:
        delete WEBSOCKET;
        break;
#endif
    }
}

int Session::init_session(void)
{    
    cmd_list = std::vector<Command>(0);
    strcpy(remain_str, "");

    // Initialize monitoring
    errors_num = 0;
    requests_num = 0;
    start_time = std::time(nullptr);
    
    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP:
        return TCPSOCKET->init();
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX:
        return UNIXSOCKET->init();
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK: 
        return WEBSOCKET->init();
#endif
    }
    
    return -1;
}

int Session::exit_session(void)
{
    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP:
        return TCPSOCKET->exit();
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX:
        return UNIXSOCKET->exit();
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK:
        return WEBSOCKET->exit();
#endif
    }
    
    return -1;
}

int Session::parse_input_buffer(int nb_bytes_rcvd)
{
    printf("Buffer:\n");
    for (int i=0; i<nb_bytes_rcvd; i++)
        printf("%u\n", buff_str[i]);

    uint16_t dev_id = buff_str[1] + (buff_str[0] << 8);
    uint16_t op_id = buff_str[3] + (buff_str[2] << 8);

    printf("dev_id = %u\n", dev_id);
    printf("op_id = %u\n", op_id);

    return 0;
}

// int Session::parse_input_buffer(void)
// {
//     // XXX Should probably preallocate a non-empty vector here...
//     cmd_list = std::vector<Command>(0);
    
//     uint32_t i = 0;
//     Command cmd;
    
//     // Set cmd
//     cmd.sess_id = id;
    
//     bool get_dev_num = 1;
//     char dev_num_str[N_CHAR_DEV];
//     char op_num_str[N_CHAR_OP];
    
//     char *remain_ptr = &buff_str[0];
    
//     // Split the buffer replacing '\n' by '\0' 
//     // and set a pointer on the following character
//     while (1) {        
//         if (buff_str[i] == '\0') {      
//             if (i>0) {
//                 assert((buff_str[i-1] != '\n' && strlen(remain_ptr) != 0) ||
//                        (buff_str[i-1] == '\0' && strlen(remain_ptr) == 0)    );
//             }
        
//             goto exit_loop;
//         }
//         else if (get_dev_num) {
//             // Get device number
//             unsigned int cnt_dev = 0;
            
//             while (1) {
//                 if (cnt_dev >= N_CHAR_DEV) {
//                     syslog_ptr->print(SysLog::CRITICAL,
//                                       "Buffer dev_num_str overflow\n");
//                     cmd.parsing_err = 1;
//                     break;
//                 }
                
//                 if (buff_str[cnt_dev+i] == '\0') {
//                     goto exit_loop;
//                 }
//                 else if (buff_str[cnt_dev+i] == '|') {
//                     dev_num_str[cnt_dev] = '\0';
//                     cnt_dev++;
//                     break;
//                 } else {
//                     dev_num_str[cnt_dev] = buff_str[cnt_dev+i];
//                     cnt_dev++;
//                 }
//             }
                
//             cmd.device = (device_t) strtoul(dev_num_str, NULL, 10);
            
//             // Get operation number
//             unsigned int cnt_op = 0;
            
//             while (1) {
//                 if (cnt_op >= N_CHAR_OP) {
//                     syslog_ptr->print(SysLog::CRITICAL, 
//                                       "Buffer op_num_str overflow\n");
//                     cmd.parsing_err = 1;
//                     break;
//                 }
            
//                 if (buff_str[cnt_op+cnt_dev+i] == '\0') {
//                     goto exit_loop;
//                 }
//                 else if (buff_str[cnt_op+cnt_dev+i] == '|') {
//                     op_num_str[cnt_op] = '\0';
//                     cnt_op++; 
//                     break;
//                 } else {
//                     op_num_str[cnt_op] = buff_str[cnt_op+cnt_dev+i];
//                     cnt_op++; 
//                 }
//             }
                
//             cmd.operation = (uint32_t) strtoul(op_num_str, NULL, 10);
            
//             if (cmd.device < device_num) {
//                 cmd.buffer = &buff_str[i + cnt_dev + cnt_op];
//             } else {
//                 syslog_ptr->print(SysLog::ERROR, "Unknown device number %u\n",
//                                   cmd.device);
//                 cmd.parsing_err = 1;
//             }
            
//             get_dev_num = 0;
//             i += cnt_dev + cnt_op;
//         } 
//         else if (buff_str[i] == '\n') {
//             buff_str[i] = '\0';
            
//             syslog_ptr->print(SysLog::DEBUG, "[R@%u] %s for device #%u\n", 
//                               id, cmd.buffer, (uint32_t)cmd.device);
                        
//             cmd_list.push_back(cmd);
//             requests_num++;
            
//             remain_ptr = &buff_str[i+1];
            
//             // Reset cmd
//             cmd.sess_id = id;
//             cmd.device = NO_DEVICE;
//             cmd.buffer = NULL;
//             cmd.parsing_err = 0;
//             cmd.status = exec_pending;
        
//             get_dev_num = 1;                
//             i++;
//         } else {
//             i++;
//         }
//     }

// exit_loop:
//     assert(strlen(remain_ptr) + 1 <= 2 * KSERVER_READ_STR_LEN);
//     strcpy(remain_str, remain_ptr);
        
//     if (cmd_list.size() == 0) // Didn't receive a full request      
//         return 1;
    
//     return 0;
// }

void Session::execute_cmds()
{
    for (unsigned int i=0; i<cmd_list.size(); i++) {
//        printf("Command #%u\n",i);
//        cmd_list[i].print();
        
        if (cmd_list[i].parsing_err == 1) {
            cmd_list[i].status = exec_skip;
            errors_num++;
        } else {       
            int exec_status 
                = session_manager.dev_manager.Execute(cmd_list[i]);
            
            if (exec_status < 0) {
                cmd_list[i].status = exec_err;
                errors_num++;
            } else {
                cmd_list[i].status = exec_done;
            }
        }
    }
}

int Session::Run()
{
    if (init_session() < 0)
        return -1;
    
    while (!session_manager.kserver.exit_comm.load()) {    
        // Read
        int nb_bytes_rcvd = -1;
        
        switch (sock_type) {
#if KSERVER_HAS_TCP
          case TCP:
            nb_bytes_rcvd = TCPSOCKET->read_data(buff_str, remain_str);
            break;
#endif
#if KSERVER_HAS_UNIX_SOCKET
          case UNIX:
            nb_bytes_rcvd = UNIXSOCKET->read_data(buff_str, remain_str);
            break;
#endif
#if KSERVER_HAS_WEBSOCKET
          case WEBSOCK:
            nb_bytes_rcvd = WEBSOCKET->read_data(buff_str, remain_str);
            break;
#endif
        }
        
        if (nb_bytes_rcvd == 0) {
            break;
        } else if (nb_bytes_rcvd < 0) {
            exit_session();
            return nb_bytes_rcvd;
        }
 
        // Parse and execute
        if (parse_input_buffer(nb_bytes_rcvd) == 1) // Request not complete
            continue;
        else
            execute_cmds();
    }

    exit_session(); 
    return 0;
}

const uint32_t* Session::RcvHandshake(uint32_t buff_size)
{
    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP:
        return TCPSOCKET->RcvHandshake(buff_size);
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX:
        return UNIXSOCKET->RcvHandshake(buff_size);
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK:
        return WEBSOCKET->RcvHandshake(buff_size);
#endif
    }
    
    return nullptr;
}

int Session::SendCstr(const char* string)
{
    switch (sock_type) {
#if KSERVER_HAS_TCP
      case TCP:
        return TCPSOCKET->SendCstr(string);
#endif
#if KSERVER_HAS_UNIX_SOCKET
      case UNIX:
        return UNIXSOCKET->SendCstr(string);
#endif
#if KSERVER_HAS_WEBSOCKET
      case WEBSOCK:
        return WEBSOCKET->SendCstr(string);
#endif
    }
    
    return -1;
}

} // namespace kserver

