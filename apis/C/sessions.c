/** 
 * Implementation of sessions.h
 *
 * (c) Koheron
 */
 
#include "sessions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Fields of the session status */
enum session_fields {
    SESS_FIELD_NONE,
    SESS_FIELD_ID,
    SESS_FIELD_CONN_TYPE,
    SESS_FIELD_CLIENT_IP,
    SESS_FIELD_CLIENT_PORT,
    SESS_FIELD_REQ_NUM,
    SESS_FIELD_ERR_NUM,
    SESS_FIELD_UPTIME,
    SESS_FIELD_PERMISSIONS,
    session_fields_num
};

/**
 * __append_session - Add a session status to the running sessions
 * @sessions: Running sessions to extend
 * @sess_status: Session status to append 
 */
static void __append_session(struct running_sessions *sessions, 
                             const struct session_status *sess_status)
{
    sessions->sessions[sessions->sess_num].sess_id = sess_status->sess_id;
    sessions->sessions[sessions->sess_num].conn_type = sess_status->conn_type;
    strcpy(sessions->sessions[sessions->sess_num].clt_ip, sess_status->clt_ip);
    sessions->sessions[sessions->sess_num].clt_port = sess_status->clt_port;
    sessions->sessions[sessions->sess_num].req_num = sess_status->req_num;
    sessions->sessions[sessions->sess_num].error_num = sess_status->error_num;
    sessions->sessions[sessions->sess_num].uptime = sess_status->uptime;
    strcpy(sessions->sessions[sessions->sess_num].permissions, sess_status->permissions);
    
    sessions->sess_num++;
}

/**
 * __get_sessions_data - Load running sessions data
 * @kcl: Kclient structure
 *
 * Returns the number of bytes received on success, -1 if failure
 */
static int __get_sessions_data(struct kclient *kcl)
{
    int bytes_read;
    
    if (kclient_send_command(kcl, 1, 4, "") < 0)
        return -1;
    
    bytes_read = kclient_rcv_esc_seq(kcl, "EORS");
    
    if (bytes_read < 0)
        return -1;
        
/*    printf("bytes_read = %u\n", bytes_read);*/
/*    printf("%s\n", kcl->rcv_buffer.buffer);*/
    
    return bytes_read;
}

struct running_sessions* kclient_get_running_sessions(struct kclient *kcl)
{
    int i, tmp_buff_cnt;
    int current_field = SESS_FIELD_ID;
    struct running_sessions *sessions;
    struct session_status tmp_session;
    char tmp_buff[2048];

    char *buffer = kcl->buffer;
    int bytes_read = __get_sessions_data(kcl);
    
    if (bytes_read < 0) {
        fprintf(stderr, "Can't get running session data\n");
        return NULL;
    }
    
    sessions = malloc(sizeof *sessions);
    
    if (sessions == NULL) {
        fprintf(stderr, "Can't allocate sessions memory\n");
        return NULL;
    }
    
    // Parse rcv_buffer
    tmp_buff[0] = '\0';
    tmp_buff_cnt = 0;
    sessions->sess_num = 0;
    
    for (i=0; i<bytes_read; i++) {
        if (buffer[i] == ':') {
            tmp_buff[tmp_buff_cnt] = '\0';
            
            switch (current_field) {
              case SESS_FIELD_ID:
                tmp_session.sess_id
                    = (int) strtol(tmp_buff, (char **)NULL, 10);

                current_field++;
                break;
              case SESS_FIELD_CONN_TYPE:
                if (strcmp(tmp_buff, "TCP") == 0) {
                    tmp_session.conn_type = TCP;
                } 
                else if (strcmp(tmp_buff, "WEBSOCK") == 0) {
                    tmp_session.conn_type = WEBSOCK;
                }
#if defined (__linux__)
                else if (strcmp(tmp_buff, "UNIX") == 0) {
                    tmp_session.conn_type = UNIX;
                }
#endif
                else {
                    fprintf(stderr, "Invalid connection type: %s\n", tmp_buff);
                    return NULL;
                }

                current_field++;
                break;
              case SESS_FIELD_CLIENT_IP:
                strcpy(tmp_session.clt_ip, tmp_buff);
                current_field++;
                break;
              case SESS_FIELD_CLIENT_PORT:
                tmp_session.clt_port
                    = (int) strtol(tmp_buff, (char **)NULL, 10);
                current_field++;
                break;
              case SESS_FIELD_REQ_NUM:
                tmp_session.req_num
                    = (int) strtol(tmp_buff, (char **)NULL, 10);
                current_field++;
                break;
              case SESS_FIELD_ERR_NUM:
                tmp_session.error_num
                    = (int) strtol(tmp_buff, (char **)NULL, 10);
                current_field++;
                break;
              case SESS_FIELD_UPTIME:
                tmp_session.uptime
                    = (time_t) strtol(tmp_buff, (char **)NULL, 10);
                current_field++;
                break;
            }
            
            tmp_buff[0] = '\0';
            tmp_buff_cnt = 0;
        } else if (buffer[i] == '\n') {
            if (strstr(tmp_buff, "EORS") != NULL)
                break;
        
            tmp_buff[tmp_buff_cnt] = '\0';
            strcpy(tmp_session.permissions, tmp_buff);
            tmp_buff[0] = '\0';
            tmp_buff_cnt = 0;
            current_field = SESS_FIELD_ID;
            
            __append_session(sessions, &tmp_session);
        } else {
            tmp_buff[tmp_buff_cnt] = buffer[i];
            tmp_buff_cnt++;
        }  
    }
    
    return sessions;
}

int kclient_is_valid_sess_id(struct running_sessions *sessions, int sid)
{
    int i;
    
    for (i=0; i<sessions->sess_num; i++)
        if (sessions->sessions[i].sess_id == sid)
            return 1;
            
    return 0;
}
