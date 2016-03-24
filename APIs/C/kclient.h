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
#include <stdint.h>

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
 
#define MAX_DEV_NUM     128
#define MAX_OP_NUM      128
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

/* Maximum length of the reception buffer */
#define RCV_BUFFER_LEN 131072

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
 * @buffer: Received data buffer
 * @current_len: Length of the buffer (including '\0' termination if any)
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

    char                 buffer[RCV_BUFFER_LEN];
    int                  current_len;
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
dev_id_t get_device_id(struct kclient *kcl, const char *dev_name);

/**
 * get_op_id - Return the ID of an operation
 * @dev_id ID of the device where to look for the operation
 * @op_name Name of the operation
 */
op_id_t get_op_id(struct kclient *kcl, dev_id_t dev_id, const char *op_name);

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
 * init_command - Initialize a command structure
 * @cmd: The command to be initialized
 * @dev_id: ID of the target device
 * @op_ref: Reference of the target operation
 */
static inline void init_command(struct command *cmd, int dev_id, int op_ref)
{   
    cmd->dev_id = dev_id;
    cmd->op_ref = op_ref;
    cmd->params_num = 0;
}
 
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

/**
 * kclient_rcv_esc_seq - Receive data until an escape sequence is reached
 * @esc_seq The escape sequence
 *
 * Returns the number of bytes read on success. -1 if failure.
 */
int kclient_rcv_esc_seq(struct kclient *kcl, const char *esc_seq);

/**
 * kclient_rcv_n_bytes - Receive a fixed number of bytes
 * @n_bytes The number of bytes to receive
 *
 * Returns the number of bytes read on success. -1 if failure.
 */
int kclient_rcv_n_bytes(struct kclient *kcl, uint32_t n_bytes);

/**
 * kclient_read_u32 - Read a little endian uint32_t
 * @rcv_uint: Pointer to the received number
 *
 * Returns the read number on success. -1 if failure.
 */
int kclient_read_u32(struct kclient *kcl, uint32_t *rcv_uint);

/**
 * kclient_read_u32 - Read a big endian uint32_t
 * @rcv_uint: Pointer to the received number
 *
 * Returns the read number on success. -1 if failure.
 */
int kclient_read_u32_big_endian(struct kclient *kcl, uint32_t *rcv_uint);

/**
 * kclient_read_int - Read a int
 * @rcv_int: Pointer to the received number
 *
 * Returns the read number on success. -1 if failure.
 */
int kclient_read_int(struct kclient *kcl, int8_t *rcv_int);

/**
 * kclient_rcv_array - Receive an array
 * @kclient Pointer to a kclient structure
 * @len Array length
 * @data_type Array data type
 *
 * Returns the number of bytes read on success. -1 if failure.
 */
#define kclient_rcv_array(kclient, len, data_type)              \
    kclient_rcv_n_bytes(kclient, sizeof(data_type) * len)

/**
 * kclient_get_buffer - Return a casted pointer to the reception buffer
 * @kclient Pointer to a kclient structure
 * @data_type Cast data type
 */
#define kclient_get_buffer(kclient, data_type)                  \
    (data_type *) kclient->buffer

/**
 * kclient_get_len - Return the length of the received array
 * @kclient Pointer to a kclient structure
 * @data_type Array data type
 */
#define kclient_get_len(kclient, data_type)                     \
    kclient->current_len / sizeof(data_type)

/**
 * kclient_send_array - Send an array using the handshaking protocol:
 *      1) The size of the buffer must have been send as a
 *         command argument to tcp-server before calling this function
 *      2) tcp-server acknowledges reception readiness by sending
 *         the number of points to receive to the client
 *      3) The client send the data buffer
 * The two last steps are implemented by the function.
 * @array_ptr Pointer to the first array element
 * @len Length of the array (number of uint32_t it contains)
 *
 * Returns the number of bytes send on success. -1 if failure.
 */
int kclient_send_array(struct kclient *kcl, uint32_t *array_ptr, uint32_t len);

#ifdef __cplusplus
}
#endif
 
#endif /* __KCLIENT_H__ */

