/** 
 * CLI/definitions.h - KServer CLI definitions
 *
 * @author Thomas Vanderbruggen <thomas@koheron.com>
 * @date 24/08/2015
 *
 * (c) Koheron 2015
 */
 
#ifndef __KS_CLI_DEFINITIONS_H__
#define __KS_CLI_DEFINITIONS_H__

/* Name of the configuration file */
#define CFG_FILENAME   ".kserver_cli"

/* Unix socket path */
#ifdef LOCAL
#  define UNIX_SOCK_PATH "/home/kserver.sock"
#else
#  define UNIX_SOCK_PATH "/var/run/kserver.sock"
#endif

/* Init tasks */
#ifdef LOCAL
#  define KSERVER_CLI_HAS_INIT_TASKS 0
#else
#  define KSERVER_CLI_HAS_INIT_TASKS 1
#endif

/* Tests */
#define KSERVER_CLI_HAS_TESTS 1
 
#endif // __KS_CLI_DEFINITIONS_H__
