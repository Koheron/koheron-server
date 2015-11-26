/** 
 * API/C/sessions.h - KServer current sessions managing
 *
 * @author Thomas Vanderbruggen <thomas@koheron.com>
 * @date 12/09/2015
 *
 * (c) Koheron 2015
 */

#ifndef __SESSIONS_H__
#define __SESSIONS_H__

#include "kclient.h"

#define IP_BUFF_LEN    128
#define PERMS_BUFF_LEN 32
#define MAX_SESS_NUM   64

/**
 * struct session_status - Status of a session
 * @sess_id: ID of the session
 * @conn_type: Session connection type
 * @clt_ip: IP address of the session client (TCP or Websocket connection)
 * @clt_port: Connection port of the client (TCP or Websocket connection)
 * @req_num: Number of requests handled by the session
 * @error_num: Number of requests terminated with error
 * @uptime: Uptime of the session
 * @permissions: Permissions of the session
 */          
struct session_status {
    int             sess_id;
    connection_t    conn_type;
    char            clt_ip[IP_BUFF_LEN];
    int             clt_port;
    int             req_num;
    int             error_num;
    time_t          uptime;
    char            permissions[PERMS_BUFF_LEN];
};

/**
 * struct running_sessions - Running sessions
 * @sess_num: Number of running sessions
 * @sessions: Running sessions status
 */
struct running_sessions {
    int                     sess_num;
    struct session_status   sessions[MAX_SESS_NUM];
};

/**
 * kclient_get_running_sessions - Obtain running sessions
 */                
struct running_sessions* kclient_get_running_sessions(struct kclient *kcl);

/**
 * kclient_is_valid_sess_id - Check the validity of a given session ID
 * @sessions: Current sessions
 * @sid: Session ID to check
 *
 * Returns 1 if the ID is valid, 0 else.
 */ 
int kclient_is_valid_sess_id(struct running_sessions *sessions, int sid);

/*
 * --------------------
 *    Session perfs
 * --------------------
 */

#define TIMING_POINT_BUFF_LEN 128
#define MAX_TIMING_POINTS_NUM 64

/**
 * struct timing_point - Performance of a timing point
 * @name: Name of the timing point
 * @duration: Mean execution duration
 */
struct timing_point {
    char  name[TIMING_POINT_BUFF_LEN];
    float mean_duration;
    int min_duration;
    int max_duration;
};

/**
 * struct session_perfs - Performances of a session
 * @sess_id: ID of the session
 * @timing_points_num: Number of timing points
 * @points: The timing points
 */
struct session_perfs {
    int                 sess_id;
    int                 timing_points_num;
    struct timing_point points[MAX_TIMING_POINTS_NUM];
};

/**
 * kclient_get_session_perfs - Obtain the performances of a session
 * @sid: ID of the session
 */ 
struct session_perfs* kclient_get_session_perfs(struct kclient *kcl, int sid);

#endif /* __SESSIONS_H__ */
