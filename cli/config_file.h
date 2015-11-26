/**
 *  CLI/config_file.h - KServer CLI config file
 *
 *  @author Thomas Vanderbruggen <thomas@koheron.com>
 *  @date 24/08/2015
 *
 *  (c) Koheron 2014-2015
 */

#ifndef __KS_CLI_CONFIG_FILE_H__
#define __KS_CLI_CONFIG_FILE_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include <kclient.h>

#include "definitions.h"

#define IP_BUFF_SIZE 128

/**
 * Maximum length of the Unix socket file path 
 *
 * Note:
 * 108 is the maximum length on Linux. See:
 * http://man7.org/linux/man-pages/man7/unix.7.html
 */
#define SOCK_PATH_SIZE 108

/**
 * connection_cfg - CLI client connection parameters
 * @type: Type of the connection
 * @ip: IP address (TCP connection only)
 * @port: Port number (TCP connection only)
 * @sock_path: Unix socket file path (Unix socket connection only)
 */
struct connection_cfg {
    connection_t type;
    char         ip[IP_BUFF_SIZE];
    int          port;
    char         sock_path[SOCK_PATH_SIZE];
};

/**
 * init_connection_cfg - Initializes a connection_cfg structure
 */
void init_connection_cfg(struct connection_cfg *conn_cfg);

/**
 * set_sock_path - Set Unix socket path
 * @conn_cfg: The connection config structure to edit
 * @sock_path: Path to the Unix socket file
 */
void set_sock_path(struct connection_cfg *conn_cfg, const char *sock_path);

/**
 * set_sock_ip - Set the IP address
 * @conn_cfg: The connection config structure to edit
 * @ip: IP address of KServer host
 */
void set_sock_ip(struct connection_cfg *conn_cfg, const char *ip);

/**
 * set_config_file - Save the connection configuration data
 * @conn_cfg: The connection config structure to save
 *
 * Returns 0 on success, -1 else
 */
int set_config_file(struct connection_cfg *conn_cfg);

/**
 * set_host_tcp - Set TCP connection parameters
 * @ip_str: IP address string
 * @port_str: Connection port string
 *
 * Returns 0 on success, and -1 if failure
 */
int set_host_tcp(const char *ip_str, const char *port_str);

/**
 * set_host_unix - Set Unix socket connection parameters
 * @sock_path: Path to the Unix socket file
 *
 * Returns 0 on success, and -1 if failure
 */
int set_host_unix(const char *sock_path);

/**
 * set_default_config - Set the config file with default settings
 *
 * Description:
 * By default an Unix socket is used with the path defined in "definitions.h"
 *
 * Returns 0 on success, -1 if failure
 */
int set_default_config();

/**
 * get_config - Load the connection configuration data from file
 * @conn_cfg: The connection config structure to load
 *
 * Returns 0 on success, -1 else
 */
int get_config(struct connection_cfg *conn_cfg);

#endif // __KS_CLI_CONFIG_FILE_H__
