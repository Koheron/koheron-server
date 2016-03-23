/** 
 * Internal header for KClient
 *
 * (c) Koheron
 */
 
#ifndef __KCLIENT_H__
#define __KCLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "definitions.h"

#include <time.h>

#if defined (__linux__)
#include <sys/socket.h>
#include <sys/types.h> 
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#elif defined (__MINGW32__)
#include <winsock2.h>
#endif

/*
 * --------- Data structures ---------
 */
 
#define MAX_DEV_NUM 128
#define MAX_OP_NUM 128
#define MAX_NAME_LENGTH 512

typedef int dev_id_t;   // Device ID type
typedef int op_id_t;    // Operation ID type

typedef enum conn_type {
    NONE,
    TCP,        // TCP socket
    WEBSOCK,    // Websocket
#if defined (__linux__)
    UNIX,       // Unix socket
#endif
    conn_type_num
} connection_t;

/**
 * struct operation - Device operation structure
 * @id: ID of the operation
 * @name: Name of the operation
 */
struct operation {
    op_id_t              id;
    char                 name[MAX_NAME_LENGTH];
};

/**
 * struct device - Device description structure
 * @id: ID of the device
 * @name: Device name
 * @ops_num: # operations for the devices
 * @ops: Device operations array
 */
struct device {
    dev_id_t             id;
    char                 name[MAX_NAME_LENGTH];
    
    int                  ops_num;
    struct operation     ops[MAX_OP_NUM];
};

/**
 * struct kclient - KServer client structure
 * @sockfd: TCP socket file descriptor
 * @serveraddr: TCP socket structure
 * @unix_sockfd: Unix socket file descriptor (Linux only)
 * @unixserveraddr: Unix socket structure (Linux only)
 * @max_num_op: Maximum number of operations among all avalaible devices
 * @devs_num: Number of available devices
 * @devices: Available devices
 * @conn_type: Connection type
 */
struct kclient {
#if defined (__linux__)
    int                  sockfd;
    struct sockaddr_in   serveraddr;
    
    int                  unix_sockfd; 
    struct sockaddr_un   unixserveraddr;
#elif defined (__MINGW32__)
    SOCKET               sockfd;
    SOCKADDR_IN          serveraddr;
#endif

    int                  max_num_op;
    int                  devs_num;
    struct device        devices[MAX_DEV_NUM];
    
    connection_t         conn_type;
};

/**
 * kclient_connect - Initialize a TCP connection with KServer
 * @host A string containing the IP address of the host
 * @port Port of KServer on the host
 *
 * Returns a pointer to a kclient structure if success, NULL otherwise
 */
struct kclient* kclient_connect(const char *host, int port);

#if defined (__linux__)
/**
 * kclient_unix_connect - Initialize a Unix socket connection with KServer
 * @sock_path Path to the Unix socket file
 *
 * Returns a pointer to a kclient structure if success, NULL otherwise
 */
struct kclient* kclient_unix_connect(const char *sock_path);
#endif
 
/**
 * kclient_shutdown - Shutdown the connection with the server
 */
void kclient_shutdown(struct kclient *kcl);

/**
 * debug_display_kclient - Display structure kclient
 */
 #ifdef DEBUG_KOHERON_API
void debug_display_kclient(struct kclient *kcl);
#endif

/**
 * get_device_id - Return the ID of a device
 * @dev_name: Device name
 */
dev_id_t get_device_id(struct kclient *kcl, char *dev_name);

/**
 * get_op_id - Return the ID of an operation
 * @dev_id ID of the device where to look for the operation
 * @op_name Name of the operation
 */
op_id_t get_op_id(struct kclient *kcl, dev_id_t dev_id, char *op_name);

/* 
 *  --------- Receive/Send ---------
 */

/* Maximum number of parameters in a command */
#define MAX_PARAMS_NUM 128

/**
 * struct command - Command to be executed by KServer
 * @dev_id: ID of the target device
 * @op_ref: ID of the operation to execute
 * @params_num: Number of parameters
 * @params: Array of stringified parameter values
 */
struct command {
    int             dev_id;
    int             op_ref;

    int             params_num;
    unsigned long   params[MAX_PARAMS_NUM];
};
 
/**
 * init_command - Allocate and initialize a new command structure
 * @dev_id: ID of the target device
 *
 * Returns NULL on failure
 */
struct command* init_command(int dev_id);
 
/**
 * add_parameter - Add a parameter to the command
 * @cmd: The command to edit
 * @param: The parameter to add
 *
 * Returns 0 on success, -1 on failure 
 */
int add_parameter(struct command *cmd, long param);

static inline void reset_command(struct command *cmd, int op_ref)
{
    cmd->op_ref = op_ref;
    cmd->params_num = 0;
}

/**
 * free_command - Desallocate a command
 */
void free_command(struct command *cmd);
 
/**
 * kclient_send - Send a command to tcp-server
 * @cmd: The command to send
 *
 * Returns 0 on success, -1 on failure
 */
int kclient_send(struct kclient *kcl, struct command *cmd);

/**
 * kclient_send_string - Send a null-terminated string to KServer
 * @str: A null-terminated string
 *
 * Returns 0 on success, -1 on failure
 */
int kclient_send_string(struct kclient *kcl, const char *str);

/* Maximum length of the reception buffer */
#define RCV_BUFFER_LEN 16384    

/**
 * struct rcv_buff - Reception buffer structure
 * @buffer: Received data buffer
 * @max_buff_len: Maximum length of the buffer (RCV_BUFFER_LEN)
 * @current_len: Length of the buffer including '\0' termination
 */
struct rcv_buff {
    char buffer[RCV_BUFFER_LEN];
    int  max_buff_len;
    int  current_len;
};

/**
 * set_rcv_buff - Set the rcv_buff to default values
 * @buff: The buffer structure to initialize
 */
void set_rcv_buff(struct rcv_buff *buff);

/**
 * kclient_rcv_esc_seq - Receive data until an escape sequence is reached
 * @rcv_buffer Pointer to a rcv_buff structure
 * @esc_seq The escape sequence
 *
 * Returns the number of bytes read on success. -1 if failure.
 */
int kclient_rcv_esc_seq(struct kclient *kcl, struct rcv_buff *rcv_buffer, char *esc_seq);

/**
 * kclient_rcv_n_bytes - Receive a fixed number of bytes
 * @rcv_buffer Pointer to a rcv_buff structure
 * @n_bytes The numbre of bytes to receive
 *
 * Returns the number of bytes read on success. -1 if failure.
 */
int kclient_rcv_n_bytes(struct kclient *kcl, struct rcv_buff *rcv_buffer, int n_bytes);

/* 
 *  --------- Kill session ---------
 */
 
int kill_session(struct kclient *kcl, int sess_id);

#ifdef __cplusplus
}
#endif
 
#endif /* __KCLIENT_H__ */

