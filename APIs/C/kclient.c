/** 
 * Implementation of kclient.h
 *
 * (c) Koheron
 */

#include "kclient.h"
#include "definitions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>

/*
 * --------- Data structures ---------
 */
 
 int get_socket_fd(struct kclient *kcl)
 {    
#if defined (__linux__)
    if (kcl->conn_type == TCP)
#endif
        return kcl->sockfd;
#if defined (__linux__)
    else // Unix socket
        return kcl->unix_sockfd;
#endif
 }

#ifdef DEBUG_KOHERON_API
void debug_display_kclient(struct kclient *kcl)
{
    int i,j;
    
    for (i=0; i<kcl->devs_num; i++) {
        printf("%s:\n", kcl->devices[i].name);
        printf("  ID: %i\n", kcl->devices[i].id);
        
        if (kcl->devices[i].ops_num > 0) {
            printf("  Operations:\n");
        
            for (j=0; j<kcl->devices[i].ops_num; j++) {
                printf("    - %s (%i)\n", 
                       kcl->devices[i].ops[j].name, 
                       kcl->devices[i].ops[j].id);
            }
        }
    }
}
#else
void debug_display_kclient(struct kclient *kcl) {}
#endif

dev_id_t get_device_id(struct kclient *kcl, const char *dev_name)
{
    int i;
    
    for (i=0; i<kcl->devs_num; i++)
        if (strcmp(kcl->devices[i].name, dev_name) == 0)
            return kcl->devices[i].id;

    return -1;
}

op_id_t get_op_id(struct kclient *kcl, dev_id_t dev_id, const char *op_name)
{
    int i, j;
    
    for (i=0; i<kcl->devs_num; i++)
        if (kcl->devices[i].id == dev_id)
            for (j=0; j<kcl->devices[i].ops_num; j++)
                if (strcmp(kcl->devices[i].ops[j].name, op_name) == 0)
                    return kcl->devices[i].ops[j].id;
    
    return -1;
}

/**
 * append_op_to_dev - Add an operation to a device
 */
static void append_op_to_dev(struct device *dev, struct operation *op)
{
    strcpy(dev->ops[dev->ops_num].name, op->name);
    dev->ops[dev->ops_num].id = dev->ops_num;
    dev->ops_num++;
}

/* 
 *  --------- Receive/Send ---------
 */

// |      RESERVED     | dev_id  |  op_id  |   payload_size    |   payload
// |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | ...

#define DEV_ID_OFFSET       4
#define OP_ID_OFFSET        6
#define PAYLOAD_SIZE_OFFSET 8
#define PAYLOAD_OFFSET      12

/**
 * Add an u16 to a buffer
 * Return the size (in bytes) of the append number
 */
static size_t append_u16(char *buff, uint16_t value)
{
    buff[0] = (value >> 8) & 0xff;
    buff[1] = value & 0xff;
    return 2;
}

/**
 * Add an u32 to a buffer
 * Return the size (in bytes) of the append number
 */
static size_t append_u32(char *buff, uint32_t value)
{
    buff[0] = (value >> 24) & 0xff;
    buff[1] = (value >> 16) & 0xff;
    buff[2] = (value >>  8) & 0xff;
    buff[3] = value & 0xff;
    return 4;
}

static size_t append_float(char *buff, float value)
{
    assert(sizeof(float) == 4);
    union { float f; uint32_t u; } __value = {value};
    return append_u32(buff, __value.u);
}

#define CMD_BUFFER_LEN 1024

int kclient_send_command(struct kclient *kcl, dev_id_t dev_id, 
                         op_id_t op_id, const char *types, ...)
{
    char   buffer[CMD_BUFFER_LEN];
    size_t buffer_len = 0;
    uint32_t payload_size = 0;
    va_list args;

    buffer_len = append_u32(buffer, 0);  // RESERVED
    buffer_len += append_u16(buffer + DEV_ID_OFFSET, dev_id);
    buffer_len += append_u16(buffer + OP_ID_OFFSET, op_id);

    va_start(args, types);
 
    while (*types != '\0') {
        uint32_t len;

        if (buffer_len >= CMD_BUFFER_LEN) {
            DEBUG_MSG("kclient_send_command: Command buffer overflow\n");
            return -1;
        }

        switch (*types) {
          case 'u':
            len = append_u32(buffer + PAYLOAD_OFFSET + payload_size,
                             va_arg(args, uint32_t));
            break;
          case 'f':
            len = append_float(buffer + PAYLOAD_OFFSET + payload_size,
                               (float)va_arg(args, double));
            break;
          default:
            fprintf(stderr, "kclient_send_command: Unknown type %c\n", *types);
            return -1;
        }

        buffer_len += len;
        payload_size += len;
        ++types;
    }
 
    va_end(args);

    buffer_len += append_u32(buffer + PAYLOAD_SIZE_OFFSET, payload_size);

    if (write(get_socket_fd(kcl), buffer, buffer_len) < 0) {
        fprintf(stderr, "kclient_send_command:Can't send command to tcp-server\n");
        return -1;
    }
    
    return 0;
}

 /**
 * set_rcv_buff - Set the rcv_buff to default values
 * @buff: The buffer structure to initialize
 */
static void set_rcv_buff(struct kclient *kcl)
{
    memset(kcl->buffer, 0, sizeof(char)*RCV_BUFFER_LEN);
    kcl->current_len = 0;
}

#define RCV_LEN 2048

// TODO Add timeout 
int kclient_rcv_esc_seq(struct kclient *kcl, const char *esc_seq)
{
    int bytes_rcv = 0;
    int bytes_read = 0;
    int num_dev = 0;
    char tmp_buff[RCV_LEN];
    set_rcv_buff(kcl);
    (kcl->buffer)[0] = '\0';
    
    while (1) {
        int i;        
        tmp_buff[0] = '\0';
                        
        bytes_rcv = recv(get_socket_fd(kcl), tmp_buff, RCV_LEN-1, 0);
        
        if (bytes_rcv == 0) {
            fprintf(stderr, "kclient_rcv_esc_seq: Connection closed by tcp-server\n");
            return -1;
        }
        
        if (bytes_rcv < 0) {
            fprintf(stderr, "kclient_rcv_esc_seq: Can't receive data\n");
            return -1;
        }
        
        if (bytes_read + bytes_rcv >= RCV_BUFFER_LEN) {
            DEBUG_MSG("kclient_rcv_esc_seq: Buffer overflow\n");
            return -1;
        }

        // Eliminates null-termination characters and 
        // concatenate result into rcv_buffer
        for (i=0; i<bytes_rcv; i++) {            
            if (tmp_buff[i] != '\0') {
                (kcl->buffer)[bytes_read] = tmp_buff[i];
                bytes_read++;
            } else if (tmp_buff[i] != '\n') {
                num_dev++;
            }
        }
        
        (kcl->buffer)[bytes_read] = '\0';
        kcl->current_len = bytes_read + 1;
        
        if (strstr(kcl->buffer, esc_seq))
            break;
    }
    
    // XXX It's a bit ugly to have this here !!
    // This is specific to kclient initialization
    // but this function can be used to other purpose !
    kcl->devs_num = num_dev-2;
    
    if (kcl->devs_num > MAX_DEV_NUM) {
        DEBUG_MSG("kclient_rcv_esc_seq: MAX_DEV_NUM overflow\n");
        return -1;
    }
    
    return bytes_read;
}

int kclient_rcv_n_bytes(struct kclient *kcl, uint32_t n_bytes)
{
    int bytes_rcv = 0;
    uint32_t bytes_read = 0;

    if (n_bytes >= RCV_BUFFER_LEN) {
        DEBUG_MSG("Receive buffer size too small\n");
        return -1;
    }

    set_rcv_buff(kcl);
    
    while (bytes_read < n_bytes) {                        
        bytes_rcv = read(kcl->sockfd, kcl->buffer + bytes_read, n_bytes - bytes_read);
        
        if (bytes_rcv == 0) {
            fprintf(stderr, "Connection closed by tcp-server\n");
            return -1;
        }
        
        if (bytes_rcv < 0) {
            fprintf(stderr, "Can't receive data\n");
            return -1;
        }

        bytes_read += bytes_rcv;
        
        if (bytes_read >= RCV_BUFFER_LEN) {
            DEBUG_MSG("Buffer overflow\n");
            return -1;
        }

        kcl->current_len = bytes_read;
    }

    assert(bytes_read == n_bytes);
    return bytes_read;
}

int kclient_read_u32(struct kclient *kcl, uint32_t *rcv_uint)
{
    if (kclient_rcv_n_bytes(kcl, sizeof(uint32_t)) < 0)
        return -1;

    *rcv_uint = (unsigned char)kcl->buffer[0] + ((unsigned char)kcl->buffer[1] << 8)
                + ((unsigned char)kcl->buffer[2] << 16) + ((unsigned char)kcl->buffer[3] << 24);
    return 0;
}

int kclient_read_u32_big_endian(struct kclient *kcl, uint32_t *rcv_uint)
{
    if (kclient_rcv_n_bytes(kcl, sizeof(uint32_t)) < 0)
        return -1;

    *rcv_uint = (unsigned char)kcl->buffer[3] + ((unsigned char)kcl->buffer[2] << 8)
                + ((unsigned char)kcl->buffer[1] << 16) + ((unsigned char)kcl->buffer[0] << 24);
    return 0;
}

int kclient_read_int(struct kclient *kcl, int32_t *rcv_int)
{
    if (kclient_rcv_n_bytes(kcl, sizeof(uint32_t)) < 0)
        return -1;

    *rcv_int = (int32_t) ((unsigned char)kcl->buffer[0] + ((unsigned char)kcl->buffer[1] << 8)
                + ((unsigned char)kcl->buffer[2] << 16) + ((unsigned char)kcl->buffer[3] << 24));
    return 0;
}

int kclient_read_bool(struct kclient *kcl, bool *rcv_bool)
{
    if (kclient_rcv_n_bytes(kcl, 4) < 0)
        return -1;

    *rcv_bool = (unsigned char)kcl->buffer[0] == 1;
    return 0;
}


int kclient_send_array(struct kclient *kcl, uint32_t *array_ptr, uint32_t len)
{
    uint32_t rcv_len;

    if (kclient_read_u32_big_endian(kcl, &rcv_len) < 0)
        return -1;

    if (rcv_len == len) {
        if (write(get_socket_fd(kcl), array_ptr, sizeof(uint32_t) * len) < 0) {
            fprintf(stderr, "Can't send array to tcp-server\n");
            return -1;
        }
    } else {
        fprintf(stderr, "Invalid handshake\n");
        return -1;
    }

    return rcv_len * sizeof(uint32_t);
}

int kclient_send_string(struct kclient *kcl, const char *str)
{
    if (write(get_socket_fd(kcl), str, strlen(str)) < 0) {
        fprintf(stderr, "Can't send command to tcp-server\n");
        return -1;
    }
    
    return 0;
}

static int kclient_get_devices(struct kclient *kcl)
{
    int line_cnt = 0;
    int i, tmp_buff_cnt;
    int bytes_read = 0;
    int dev_cnt = 0;
    bool is_dev_id = 0, is_dev_name = 0;
    struct operation tmp_op;
    char tmp_buff[2048];

    if (kclient_send_command(kcl, 1, 1, "") < 0)
        return -1;
    
    bytes_read = kclient_rcv_esc_seq(kcl, "EOC");
    
    if (bytes_read < 0)
        return -1;
    
    // Parse rcv_buffer
    tmp_buff[0] = '\0';
    tmp_buff_cnt = 0;
    kcl->devices[dev_cnt].ops_num = 0;
    
    for (i=0; i<bytes_read; i++) {
        if (line_cnt == 0) {
            if (kcl->buffer[i] != '\n') {
                tmp_buff[tmp_buff_cnt] = kcl->buffer[i];
                tmp_buff_cnt++;
            } else {
                tmp_buff[tmp_buff_cnt] = '\0';
                kcl->max_num_op = (int) strtol(tmp_buff, (char **)NULL, 10);
                
                if (kcl->max_num_op > MAX_OP_NUM) {
                    DEBUG_MSG("MAX_OP_NUM overflow\n");
                    return -1;
                }
                              
                tmp_buff[0] = '\0';
                tmp_buff_cnt = 0;
                line_cnt++;
                is_dev_id = 1;
            }
        } else {
            if (kcl->buffer[i] == ':') {
                if (is_dev_id) {
                    tmp_buff[tmp_buff_cnt] = '\0';
                    kcl->devices[dev_cnt].id 
                        = (int) strtol(tmp_buff, (char **)NULL, 10);
                    tmp_buff[0] = '\0';
                    tmp_buff_cnt = 0;
                    is_dev_id = 0;
                    is_dev_name = 1;
                } else if (is_dev_name) {
                    tmp_buff[tmp_buff_cnt] = '\0';
                    strcpy(kcl->devices[dev_cnt].name, tmp_buff);
                    tmp_buff[0] = '\0';
                    tmp_buff_cnt = 0;
                    is_dev_id = 0;
                    is_dev_name = 0;
                } else { // is_op_name
                    if (tmp_buff_cnt != 0) {
                        tmp_buff[tmp_buff_cnt] = '\0';
                        strcpy(tmp_op.name, tmp_buff);
                        append_op_to_dev(&(kcl->devices[dev_cnt]), &tmp_op);
                        tmp_buff[0] = '\0';
                        tmp_buff_cnt = 0;
                    }
                }
            } else if (kcl->buffer[i] == '\n') {
                if (tmp_buff_cnt != 0) {
                    tmp_buff[tmp_buff_cnt] = '\0';
                    strcpy(tmp_op.name, tmp_buff);
                    append_op_to_dev(&(kcl->devices[dev_cnt]), &tmp_op);
                    tmp_buff[0] = '\0';
                    tmp_buff_cnt = 0;
                }
                
                if (dev_cnt+1 == kcl->devs_num)
                    break;
                
                dev_cnt++;
                kcl->devices[dev_cnt].ops_num = 0;
                is_dev_id = 1;
                line_cnt++;                
            } else {
                tmp_buff[tmp_buff_cnt] = kcl->buffer[i];
                tmp_buff_cnt++;
            }
        }
    }
    
#if VERBOSE
    debug_display_kclient(kcl);
#endif
    
    return 0;
}
 
#if defined (__linux__)
static int open_kclient_tcp_socket(struct kclient *kcl)
{
    kcl->sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (kcl->sockfd < 0) {
        fprintf(stderr, "Can't open socket\n");		
        return -1;
    }

    return 0;
}
#elif defined (__MINGW32__)
static int open_kclient_tcp_socket(struct kclient *kcl)
{
    WSADATA WsaDat;
    SOCKET Socket;

    kcl->sockfd = INVALID_SOCKET;

    if (WSAStartup(MAKEWORD(2,2), &WsaDat) != 0) {
        fprintf(stderr, "Winsock initialization failed\n");
        WSACleanup();
        return -1;
    }

    Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (Socket == INVALID_SOCKET) {
        fprintf(stderr, "Can't open socket\n");
        WSACleanup();
        return  -1;
    }

    kcl->sockfd = Socket;
    return 0;
}
#endif

#if defined (__linux__)
static int open_kclient_unix_socket(struct kclient *kcl)
{
    kcl->unix_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (kcl->unix_sockfd < 0) {
        fprintf(stderr, "Can't open Unix socket\n");		
        return -1;
    }

    return 0;
}
#endif // (__linux__)

static void set_serveraddr(struct kclient *kcl, const char *host, int port)
{    
    memset(&(kcl->serveraddr), 0, sizeof(kcl->serveraddr));
    (kcl->serveraddr).sin_family = AF_INET;
    (kcl->serveraddr).sin_addr.s_addr = inet_addr(host);
    (kcl->serveraddr).sin_port = htons(port);
}

#if defined (__linux__)
static void set_unixserveraddr(struct kclient *kcl, const char *sock_path)
{    
    (kcl->unixserveraddr).sun_family = AF_UNIX;
    strcpy((kcl->unixserveraddr).sun_path, sock_path);
}

static int unix_socket_connect(struct kclient *kcl)
{
    int len = strlen((kcl->unixserveraddr).sun_path) 
                    + sizeof((kcl->unixserveraddr).sun_family);

    if (connect(kcl->unix_sockfd, 
                (struct sockaddr *)&kcl->unixserveraddr, len) < 0) {
        close(kcl->unix_sockfd);
        return -1;
    }

    return 0;
}
#endif // (__linux__)

#if defined (__linux__)
static int set_kclient_sock_options(struct kclient *kcl)
{
    int on = 1;

    if (connect(kcl->sockfd, (struct sockaddr*) &(kcl->serveraddr), 
                sizeof(kcl->serveraddr)) < 0) {
        close(kcl->sockfd);
        return -1;
    }

    if (setsockopt(kcl->sockfd, IPPROTO_TCP, TCP_NODELAY, 
                   (const char *)&on, sizeof(int)) < 0) {
        fprintf(stderr, "Can't set TCP_NODELAY option\n");
        close(kcl->sockfd);
        return -1;
    }

    return 0;
}
#elif defined (__MINGW32__)
static int set_kclient_sock_options(struct kclient *kcl)
{
    int on = 1;

    if (connect(kcl->sockfd, (SOCKADDR*)(&(kcl->serveraddr)), 
                sizeof(kcl->serveraddr)) != 0) {
        WSACleanup();
        return -1;
    }

    if (setsockopt(kcl->sockfd, IPPROTO_TCP, TCP_NODELAY, 
                   (const char *)&on, sizeof(int)) < 0) {
        fprintf(stderr, "Can't set TCP_NODELAY option\n");
        WSACleanup();
        return -1;
    }

    return 0;
}
#endif

KOHERON_LIB_EXPORT
struct kclient* kclient_connect(const char *host, int port)
{
    struct kclient *kcl = malloc(sizeof *kcl);

    if (kcl == NULL) {
        fprintf(stderr, "Can't allocate Kclient memory\n");
        return NULL;
    }

    kcl->conn_type = TCP;
#if defined (__linux__)
    kcl->unix_sockfd = -1;
#endif


    if (open_kclient_tcp_socket(kcl) < 0)
        return NULL;

    set_serveraddr(kcl, host, port);


    if (set_kclient_sock_options(kcl) < 0)
        return NULL;

    if (kclient_get_devices(kcl) < 0)
        return NULL;

    return kcl;
}

#if defined (__linux__)
KOHERON_LIB_EXPORT
struct kclient* kclient_unix_connect(const char *sock_path)
{
    struct kclient *kcl = malloc(sizeof *kcl);

    if (kcl == NULL) {
        fprintf(stderr, "Can't allocate Kclient memory\n");
        return NULL;
    }

    kcl->conn_type = UNIX;
    kcl->sockfd = -1;

    if (open_kclient_unix_socket(kcl) < 0)
        return NULL;

    set_unixserveraddr(kcl, sock_path);

    if (unix_socket_connect(kcl) < 0) 
        return NULL;

    if (kclient_get_devices(kcl) < 0)
        return NULL;

    return kcl;
}
#endif // (__linux__)

#if defined (__linux__)
static void close_kclient_socket(struct kclient *kcl)
{
    if (kcl->conn_type == TCP && kcl->sockfd >= 0)
        close(kcl->sockfd);
    
    if (kcl->conn_type == UNIX && kcl->unix_sockfd >= 0)
        close(kcl->unix_sockfd);
}
#elif defined (__MINGW32__)
static void close_kclient_socket(struct kclient *kcl)
{
    if (kcl->sockfd != INVALID_SOCKET) {
        shutdown(kcl->sockfd, SD_SEND);
        closesocket(kcl->sockfd);
        WSACleanup();
    }
}
#endif

KOHERON_LIB_EXPORT
void kclient_shutdown(struct kclient *kcl)
{    
    if (kcl != NULL) {
        close_kclient_socket(kcl);
        free(kcl);
        kcl = NULL;
    }
}

