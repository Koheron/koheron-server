/** 
 * Public header for Koheron API library
 *
 * (c) Koheron
 */
 
#ifndef __KOHERON_H__
#define __KOHERON_H__
 
#ifdef __cplusplus
extern "C" {
#endif

/* 
 * ---------------
 *     KClient
 * ---------------
 */
 
struct kclient;
 
/**
 * kclient_connect - Initialize a TCP connection with KServer
 * @host A string containing the IP address of the host
 * @port Port of KServer on the host
 *
 * Returns a pointer to a kclient structure if success, NULL otherwise
 */
struct kclient* kclient_connect(char *host, int port);

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

/* 
 * ---------------
 *     Devmem
 * ---------------
 */
 
struct devmem;

/**
 * dev_mem_init - Initialize the Devmem device
 * @kcl A previously initialized pointer to a kclient structure
 *
 * Returns a pointer to a devmem structure if success, NULL otherwise
 */
struct devmem* dev_mem_init(struct kclient *kcl);

/**
 * dev_mem_exit - Exit the Devmem device
 */
void dev_mem_exit(struct devmem *dvm);

#ifdef __cplusplus
}
#endif
 
#endif /* __KOHERON_H__ */
