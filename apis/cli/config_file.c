/**
 *  cli/config_file.c - Implementation of config_file.h
 *
 *  (c) Koheron
 */
 
#include "config_file.h"
#include "definitions.h"

#include <unistd.h>

void init_connection_cfg(struct connection_cfg *conn_cfg)
{
    conn_cfg->type = NONE;
    memset(conn_cfg->ip, 0, IP_BUFF_SIZE);
    conn_cfg->port = -1;
    memset(conn_cfg->sock_path, 0, SOCK_PATH_SIZE);
}

void set_sock_path(struct connection_cfg *conn_cfg, const char *sock_path)
{
    assert(strlen(sock_path) < SOCK_PATH_SIZE);
    memset(conn_cfg->sock_path, 0, SOCK_PATH_SIZE);
    strcpy(conn_cfg->sock_path, sock_path);
}

void set_sock_ip(struct connection_cfg *conn_cfg, const char *ip)
{
    assert(strlen(ip) < IP_BUFF_SIZE);
    memset(conn_cfg->ip, 0, IP_BUFF_SIZE);
    strcpy(conn_cfg->ip, ip);
}

int set_config_file(struct connection_cfg *conn_cfg)
{
    FILE *f = fopen(CFG_FILENAME, "w");
    
    if (f == NULL) {
        fprintf(stderr, "Cannot open config file\n");
        return -1;
    }
    
    fprintf(f, "type: %i\n", conn_cfg->type);
    
    if (conn_cfg->type == TCP) {
        fprintf(f, "ip: %s\n", conn_cfg->ip);
        fprintf(f, "port: %i\n", conn_cfg->port);
    }
    else if (conn_cfg->type == UNIX) {
        fprintf(f, "sock_path: %s\n", conn_cfg->sock_path);
    } else {
        fprintf(stderr, "Invalid connection type\n");
        return -1;
    }
    
    fclose(f);
    
    return 0;
}

int set_host_tcp(const char *ip_str, const char *port_str)
{
    struct connection_cfg conn_cfg;
    init_connection_cfg(&conn_cfg);
    conn_cfg.type = TCP;
    set_sock_ip(&conn_cfg, ip_str);
        
    if (strcmp(conn_cfg.ip, "localhost") == 0)
        strcpy(conn_cfg.ip, "127.0.0.1");
        
    conn_cfg.port = (int) strtoul(port_str, NULL, 10);
        
    return set_config_file(&conn_cfg);
}

int set_host_unix(const char *sock_path)
{
    struct connection_cfg conn_cfg;
    init_connection_cfg(&conn_cfg);
    conn_cfg.type = UNIX;
    set_sock_path(&conn_cfg, sock_path);
        
    return set_config_file(&conn_cfg);
}

int set_default_config()
{
    return set_host_unix(UNIX_SOCK_PATH);
}

/**
 * key_value_pair - A structure to store a key-value pair
 */
struct key_value_pair {
    char *key;
    char *value;
};

/**
 * __get_key_value_pair - Load a key-value pair from a line
 * @kv_pair: The key-value pair to load
 * @line: The line to tokenize
 *
 * Description:
 * Tokenize a line "key : value\n" and store the tokens
 *
 * Returns 0 on success, -1 if failure
 */
static int __get_key_value_pair(struct key_value_pair *kv_pair, char *line)
{
    char *ptr = strchr(line, ':');
        
    if (ptr == NULL)
        return -1;

    kv_pair->value = ptr + 2;
    (kv_pair->value)[strlen(kv_pair->value)-1] = '\0'; // Remove '\n' at the end
    *ptr = '\0';
    kv_pair->key = line;

    return 0;
}

/**
 * __parse_pairs - Parse a key-value pair and edit the configuration
 * @conn_cfg: The configuration to edit
 * @kv_pair: The key-value pair to parse
 */
static int __parse_pairs(struct connection_cfg *conn_cfg, 
                         struct key_value_pair *kv_pair)
{
    if (strcmp(kv_pair->key, "type") == 0) {
        int conn_type = (int) strtoul(kv_pair->value, NULL, 10);
       
        if (conn_type >= conn_type_num) {
            fprintf(stderr, "Invalid connection type\n");
            return -1;
        }
      
        conn_cfg->type = conn_type;
    }
    else if (strcmp(kv_pair->key, "ip") == 0) {    
        if (conn_cfg->type != TCP) {
            fprintf(stderr, "Unexpected ip key for non TCP connection\n");
            return -1;
        }
  
        set_sock_ip(conn_cfg, kv_pair->value);
    } 
    else if (strcmp(kv_pair->key, "port") == 0) {
        if (conn_cfg->type != TCP) {
            fprintf(stderr, "Unexpected port key for non TCP connection\n");
            return -1;
        }

        conn_cfg->port = (int) strtoul(kv_pair->value, NULL, 10);
    } 
    else if (strcmp(kv_pair->key, "sock_path") == 0) {
        if (conn_cfg->type != UNIX) {
            fprintf(stderr, "Unexpected sock_path key "
                            "for non Unix socket connection\n");
            return -1;
        }

        set_sock_path(conn_cfg, kv_pair->value);
    } else {
        fprintf(stderr, "Invalid key %s\n", kv_pair->key);
        return -1;
    }

    return 0;
}

int get_config(struct connection_cfg *conn_cfg)
{
    FILE *f;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    
    /*
     * Check whether the config file exists.
     * If not generate a default configuration.
     */
    if (access(CFG_FILENAME, F_OK) < 0)
        if (set_default_config() < 0)
            return -1;

    f = fopen(CFG_FILENAME, "r");
    
    if (f == NULL) {
        fprintf(stderr, "Cannot open config file\n");
        return -1;
    }
    
    init_connection_cfg(conn_cfg);
    
    while ((read = getline(&line, &len, f)) != -1) {
        struct key_value_pair kv_pair;
        
        if (__get_key_value_pair(&kv_pair, line) < 0)
            goto exit_error;
        
/*        printf("(key = %s, value = %s)\n", kv_pair.key, kv_pair.value);*/
        
        if (__parse_pairs(conn_cfg, &kv_pair) < 0)
            goto exit_error;
    }

    free(line);
    fclose(f);
    
    return 0;
    
exit_error:
    fprintf(stderr, "Corrupted config file\n");
    
    free(line);
    fclose(f);
    
    return -1;
}

