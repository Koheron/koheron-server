/**
 *  cli/kserver_cli.c - Command line interface for KServer
 *
 *  (c) Koheron
 */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include <kclient.h>
#include <sessions.h>
#include <koheron.h>

#include "definitions.h"
#include "config_file.h"

/**
 * TEST_CMD - Test a command value
 * @short: Short option name
 * @long: Long option name
 * 
 * Description:
 * Check whether argv[1] is equal to either 
 * a short or a long option descriptor
 */
#define TEST_CMD(short, long) \
  ((strcmp(argv[1], (short)) == 0) || (strcmp(argv[1], (long)) == 0))

/**
 * __start_client - Initiate connection with KServer
 *
 * Returns a pointer to a kclient structure
 *
 * Note:
 * __stop_client must be called to close the connection
 */
struct kclient* __start_client()
{
    struct kclient *kcl;
    struct connection_cfg conn_cfg;

    if (get_config(&conn_cfg) < 0) {
        fprintf(stderr, "Error in configuration file\n");
        return NULL;
    }

    if (conn_cfg.type == TCP) {
        kcl = kclient_connect(conn_cfg.ip, conn_cfg.port);
    }
    else if (conn_cfg.type == UNIX) {
        kcl = kclient_unix_connect(conn_cfg.sock_path);
    } else {
        fprintf(stderr, "Invalid connection type\n");
        return NULL;
    }
    
    return kcl;
}

/**
 * __stop_client - Close a connection
 * @kcl: KClient structure initialized by __start_client
 */
void __stop_client(struct kclient *kcl)
{
    kclient_shutdown(kcl);
}

/* 
 *  -------------------------------------------------
 *                      Status
 *  -------------------------------------------------
 */
 
/* Status options */
#define IS_STATUS_HELP     TEST_CMD("-h", "--help")
#define IS_STATUS_DEVICES  TEST_CMD("-d", "--devices")
#define IS_STATUS_SESSIONS TEST_CMD("-s", "--sessions")
#define IS_STATUS_PERFS    TEST_CMD("-p", "--perfs")

/**
 * __status_usage - Help for the status command
 */
void __status_usage(void)
{
    printf("KServer status\n\n");
    printf("Usage: kserver status [Option] [Args...]\n\n");
    printf("\e[7m%-10s%-15s%-15s%-50s\e[27m\n", "Option", "Long option", 
           "Args", "Meaning");

    printf("%-10s%-15s%-15s%-50s\n", "-h", "--help", "", "Show this message");
    printf("%-10s%-15s%-15s%-50s\n", "-d", "--devices", "", 
           "Show the available devices");
    printf("%-10s%-15s%-15s%-50s\n", "-d", "--devices", "DEVICE", 
           "Show the available commands for DEVICE");
    printf("%-10s%-15s%-15s%-50s\n", "-s", "--sessions", "", 
           "Show the running sessions");
    printf("%-10s%-15s%-15s%-50s\n", "-p", "--perfs", "SID", 
           "Show the perfs of the session with ID SID");
}

/**
 * __display_devices - Display the available devices
 * @kcl: KClient data structure
 */
void __display_devices(struct kclient *kcl)
{
    unsigned int i;

    printf("\e[7m%-5s%-15s%-50s\e[27m\n", "ID", "DEVICE", "#OPS");

    for (i=1; i<kcl->devs_num; i++) {
        printf("%-5u%-15s%-50u\n", kcl->devices[i].id, 
               kcl->devices[i].name, kcl->devices[i].ops_num);
    }
}

/**
 * __display_operations - Display the available operations
 * @kcl: KClient data structure
 */
void __display_operations(struct kclient *kcl, char *dev_name)
{
    unsigned int i;
    dev_id_t id = get_device_id(kcl, dev_name);

    for (i=1; i<kcl->devs_num; i++) {
        if (kcl->devices[i].id == id) {
            unsigned int j;

            printf("\e[7m%-5s%-50s\e[27m\n", "ID", "OPERATION");
            
            for (j=0; j<kcl->devices[i].ops_num; j++) {
                printf("%-5u%-50s\n", kcl->devices[i].ops[j].id, 
                                      kcl->devices[i].ops[j].name);
            }
            
            goto exit_success;
        }
    }

    fprintf(stderr, "Unknown device %s\n", dev_name);
    exit(EXIT_FAILURE);

exit_success:
    return;
}

#define TIME_STR_LEN 128

/**
 * __format_time - Format the time duration since start_time in H:m:s
 * @time_str: The string to store the formated date
 * @start_time: Start time
 */
void __format_time(char *time_str, time_t uptime)
{
    memset(time_str, 0, TIME_STR_LEN);

    int hours = uptime / 3600; 
    int remainder = uptime % 3600;
    int minutes = remainder / 60;
    int seconds = remainder % 60;
    
    snprintf(time_str, TIME_STR_LEN, "%i:%02i:%02i", hours, minutes, seconds);
}

void __display_perfs(struct session_perfs *perfs)
{
    int i;
    struct timing_point pt;
    
    printf("\e[7m%-15s%-15s%-15s%-10s\e[27m\n",
           "TIME PT", "MEAN", "MIN", "MAX");

    for (i=0; i<perfs->timing_points_num; i++) {
        pt = perfs->points[i];
        
        printf("%-15s%-15f%-15d%-10d\n",
               pt.name, pt.mean_duration, pt.min_duration, pt.max_duration);
    }
}

/**
 * __display_sessions - Display running sessions
 * @sessions: Running sessions structure
 */
void __display_sessions(struct running_sessions *sessions)
{
    int i;
    
    printf("\e[7m%-5s%-15s%-15s%-10s%-10s%-10s%-10s%-50s\e[27m\n",
           "SID", "CONNECTION", "IP", "PORT", "#REQ",
           "#ERR", "PERMS", "TIME (H:M:S)");
    
    for (i=0; i<sessions->sess_num; i++) {
        const char *sock_type_name;
        const char *ip = sessions->sessions[i].clt_ip;
        
        char time_str[TIME_STR_LEN];
        
        switch (sessions->sessions[i].conn_type) {
          case TCP:
            sock_type_name = "TCP";
            break;
          case WEBSOCK:
            sock_type_name = "WEBSOCK";
            break;
          case UNIX:
            sock_type_name = "UNIX";
            ip = "--";
            break;
          default:
            fprintf(stderr, "Invalid connection type\n");
        }
        
        __format_time(time_str, sessions->sessions[i].uptime);
    
        printf("%-5u%-15s%-15s%-10u%-10u%-10u%-10s%-50s\n", 
               sessions->sessions[i].sess_id,
               sock_type_name,
               ip,
               sessions->sessions[i].clt_port,
               sessions->sessions[i].req_num,
               sessions->sessions[i].error_num,
               sessions->sessions[i].permissions,
               time_str);
    }
}

/**
 * ks_cli_status - Execute status operations
 */
void ks_cli_status(int argc, char **argv)
{
    assert(strcmp(argv[0], "status") == 0);
    
    if (argc < 2) {
        fprintf(stderr, "Available commands:\n\n");
        __status_usage();
        exit(EXIT_FAILURE);
    }
    
    if (IS_STATUS_DEVICES) {
        struct kclient *kcl = __start_client();
        
        if (kcl == NULL) {
            fprintf(stderr, "Connection failed\n");
            exit(EXIT_FAILURE);
        }    
        
        switch (argc) {
          case 2: // No argument: display available devices
            __display_devices(kcl);
            break;
          case 3: // Device name argument: display device operations
            __display_operations(kcl, argv[2]);
            break;
          default:
            fprintf(stderr, "Invalid number of arguments\n");
            __stop_client(kcl);
            exit(EXIT_FAILURE);
        }
        
        __stop_client(kcl);
    }
    else if (IS_STATUS_HELP) {
        __status_usage();
        exit(EXIT_SUCCESS);
    }
    else if (IS_STATUS_SESSIONS) {
        struct kclient *kcl = __start_client();
        
        if (kcl == NULL) {
            fprintf(stderr, "Connection failed\n");
            exit(EXIT_FAILURE);
        }  
        
        struct running_sessions *sessions = kclient_get_running_sessions(kcl);
        
        if (sessions == NULL) {
            __stop_client(kcl);
            exit(EXIT_FAILURE);
        }
            
        __display_sessions(sessions);

        free(sessions);
        __stop_client(kcl);
    }
    else if (IS_STATUS_PERFS) {
        struct kclient *kcl;
        struct running_sessions *sessions;
        struct session_perfs *perfs;
        int sid;
        
        if (argc != 3) {
            fprintf(stderr, "Invalid number of arguments.\n"
                            "Expect session ID\n");
            exit(EXIT_FAILURE);
        }
        
        sid = (int) strtol(argv[2], (char **)NULL, 10);
        
        kcl = __start_client();
        
        if (kcl == NULL) {
            fprintf(stderr, "Connection failed\n");
            exit(EXIT_FAILURE);
        }
        
        sessions = kclient_get_running_sessions(kcl);
        
        if (sessions == NULL) {
            __stop_client(kcl);
            exit(EXIT_FAILURE);
        }
        
        if (!kclient_is_valid_sess_id(sessions, sid)) {
            fprintf(stderr, "Invalid session ID %d\n", sid);
            free(sessions);
            __stop_client(kcl);
            exit(EXIT_FAILURE);
        }
        
        perfs = kclient_get_session_perfs(kcl, sid);
        
        if (perfs == NULL) {
            __stop_client(kcl);
            exit(EXIT_FAILURE);
        }
                
        __display_perfs(perfs);
        
        free(sessions);
        free(perfs);
        __stop_client(kcl);        
    } else {
        fprintf(stderr, "Invalid status command: %s\n", argv[1]);
        __status_usage();
        exit(EXIT_FAILURE);
    }
}

/* 
 *  -------------------------------------------------
 *                      Host
 *  -------------------------------------------------
 */

/**
 * __host_usage - Help for the host command
 */
void __host_usage(void)
{
    printf("Configure socket to connect to KServer daemon\n\n");
    printf("Usage: kserver host [Option...]\n\n");
    printf("\e[7m%-10s%-20s%-50s\e[27m\n", "Option", "Long option", "Meaning");

    printf("%-10s%-20s%-50s\n", "-h", "--help", "Show this message");
    printf("%-10s%-20s%-50s\n", "-t", "--tcp", "Set a TCP connection");
    printf("%-10s%-20s%-50s\n", "-u", "--unix", "Set a Unix socket connection");
    printf("%-10s%-20s%-50s\n", "-d", "--default", "Set the default settings");
    printf("%-10s%-20s%-50s\n", "-s", "--status", "Show the current settings");
}

/* Host options */
#define IS_HOST_TCP     TEST_CMD("-t", "--tcp")
#define IS_HOST_UNIX    TEST_CMD("-u", "--unix")
#define IS_HOST_STATUS  TEST_CMD("-s", "--status")
#define IS_HOST_DEFAULT TEST_CMD("-d", "--default")
#define IS_HOST_HELP    TEST_CMD("-h", "--help")

/**
 * __display_config_status - Show CLI client connection settings
 *
 * Returns 0 on success, and -1 if failure 
 */
int __display_config_status()
{
    struct connection_cfg conn_cfg;
        
    if (get_config(&conn_cfg) < 0) {
        fprintf(stderr, "Error in configuration file\n");
        return -1;
    }
    
    if (conn_cfg.type == TCP) {
        printf("\e[7m%-20s%-20s%-50s\e[27m\n","Connection", "IP", "Port");
        printf("%-20s%-20s%-50u\n", "TCP", conn_cfg.ip, conn_cfg.port);
    } else if (conn_cfg.type == UNIX) {
        printf("\e[7m%-20s%-50s\e[27m\n","Connection", "Socket path");
        printf("%-20s%-50s\n", "Unix socket", conn_cfg.sock_path);
    } else {
        fprintf(stderr, "Invalid connection type\n");
        return -1;
    }
    
    return 0;
}

/**
 * ks_cli_host - Execute host operations
 */
void ks_cli_host(int argc, char **argv)
{    
    assert(strcmp(argv[0], "host") == 0);
    
    if (argc < 2) {
        fprintf(stderr, "Available commands:\n\n");
        __host_usage();
        exit(EXIT_FAILURE);
    }
    
    if (IS_HOST_TCP) {    
        if (argc != 4) {
            fprintf(stderr, "Invalid number of argument(s).\n"
                            "Please provide IP address and port number\n");
            exit(EXIT_FAILURE);
        }
        
        if (set_host_tcp(argv[2], argv[3]) < 0)
            exit(EXIT_FAILURE);       
    }
    else if (IS_HOST_UNIX) {
        int err = -1;
        
        switch (argc) {
          case 2: // Use default path
            err = set_host_unix(UNIX_SOCK_PATH);
            break;
          case 3:
            err = set_host_unix(argv[2]);
            break;
          default:
            fprintf(stderr, "Invalid number of argument(s)\n");
            exit(EXIT_FAILURE);
        }
        
        if (err < 0)
            exit(EXIT_FAILURE);
    }
    else if (IS_HOST_DEFAULT) {
        if (set_default_config() < 0)
            exit(EXIT_FAILURE);
    }   
    else if (IS_HOST_STATUS) {
        if(__display_config_status() < 0)
            exit(EXIT_FAILURE);
    }
    else if (IS_HOST_HELP) {
        __host_usage();
        exit(EXIT_SUCCESS);
    } else {
        fprintf(stderr, "Invalid set command: %s\n", argv[1]);
        __host_usage();
        exit(EXIT_FAILURE);
    }
}

/* 
 *  -------------------------------------------------
 *                      Kill
 *  -------------------------------------------------
 */

/**
 * __kill_usage - Help for the kill command
 */
void __kill_usage(void)
{
    printf("Kill a session\n");
    printf("Usage: kserver kill session_id\n");
}

/* Kill options */
#define IS_KILL_HELP   TEST_CMD("-h", "--help")

/**
 * ks_cli_kill - Execute kill operations
 */
void ks_cli_kill(int argc, char **argv)
{
    int sess_id;

    if (argc != 2) {
        fprintf(stderr, "Available commands:\n");
        __kill_usage();
        exit(EXIT_FAILURE);
    }
    
    if (IS_KILL_HELP) {
        __kill_usage();
        exit(EXIT_SUCCESS);
    }
    
    sess_id = (int) strtoul(argv[1], NULL, 10);
    
    struct kclient *kcl = __start_client();
    
    if (kcl == NULL) {
        fprintf(stderr, "Connection failed\n");
        exit(EXIT_FAILURE);
    } 
    
    if (kill_session(kcl, sess_id) < 0) {
        fprintf(stderr, "Cannot kill session %i\n", sess_id);
        __stop_client(kcl);
        exit(EXIT_FAILURE);
    }
    
    __stop_client(kcl);
    
    exit(EXIT_SUCCESS);
}

/* 
 *  -------------------------------------------------
 *                   Init tasks
 *  -------------------------------------------------
 */
 
#if KSERVER_CLI_HAS_INIT_TASKS

/**
 * __init_tasks_usage - Help for the init_tasks command
 */
void __init_tasks_usage(void)
{
    printf("Tasks to be performed at system initialization\n");
    printf("Usage: kserver init_tasks [option]\n\n");
    
    printf("\e[7m%-10s%-20s%-50s\e[27m\n", "Option", "Long option", "Meaning");

    printf("%-10s%-20s%-50s\n", "-h", "--help", "Show this message");
    printf("%-10s%-20s%-50s\n", "-i", "--ip_on_leds", 
           "Display last IP number onto RedPitaya LEDs");
}

int __set_ip_on_leds(uint32_t leds_addr)
{
    dev_id_t dev_id;
    op_id_t op_id;
    struct command* cmd;
    
    struct kclient *kcl = __start_client();
    
    if (kcl == NULL) {
        fprintf(stderr, "Connection failed\n");
        return -1;
    }
    
    if ((dev_id = get_device_id(kcl, "INIT_TASKS")) < 0) {
        fprintf(stderr, "Unknown device INIT_TASKS\n");
        return -1;
    }
    
    if ((op_id = get_op_id(kcl, dev_id, "SHOW_IP_ON_LEDS")) < 0) {
        fprintf(stderr, "Unknown operation SHOW_IP_ON_LEDS\n");
        return -1;
    }
    
    if ((cmd = init_command(dev_id)) == NULL) {
        fprintf(stderr, "Cannot allocate show_ip_leds command\n");
        return -1;
    }
    
    cmd->op_ref = op_id;
    add_parameter(cmd, (long)leds_addr);
    
    if (kclient_send(kcl, cmd) < 0) {
        fprintf(stderr, "Cannot send show_ip_leds command\n");
        return -1;
    }

    free_command(cmd);
    __stop_client(kcl);
    
    return 0;
}

/* Init_tasks options */
#define IS_INIT_TASKS_HELP          TEST_CMD("-h", "--help")
#define IS_INIT_TASKS_IP_ON_LEDS    TEST_CMD("-i", "--ip_on_leds")

/**
 * ks_cli_init_tasks - Execute init tasks
 */
void ks_cli_init_tasks(int argc, char **argv)
{   
    uint32_t leds_addr;
     
    assert(strcmp(argv[0], "init_tasks") == 0);
    
    if (argc < 2) {
        fprintf(stderr, "Available commands:\n\n");
        __init_tasks_usage();
        exit(EXIT_FAILURE);
    }
    
    if (IS_INIT_TASKS_IP_ON_LEDS) {
        if (argc == 2) {
            leds_addr = 0x60000000;
        }
        else if (argc == 3) {
            leds_addr = (int) strtol(argv[2], (char **)NULL, 0);
        } else {
            fprintf(stderr, "Invalid number of arguments\n");
            exit(EXIT_FAILURE);
        }
        
        if (__set_ip_on_leds(leds_addr) < 0)
            exit(EXIT_FAILURE);
    }
    else if (IS_INIT_TASKS_HELP) {
        __init_tasks_usage();
        exit(EXIT_SUCCESS);
    } else {
        fprintf(stderr, "Invalid set command: %s\n", argv[1]);
        __init_tasks_usage();
        exit(EXIT_FAILURE);
    }
}

#endif /* KSERVER_CLI_HAS_INIT_TASKS */

/* 
 *  -------------------------------------------------
 *                   Tests
 *  -------------------------------------------------
 */
 
#if KSERVER_CLI_HAS_TESTS

/**
 * __tests_usage - Help for the tests command
 */
void __tests_usage(void)
{
    printf("Tests for development purpose\n");
    printf("Usage: kserver tests [option]\n\n");
    
    printf("\e[7m%-10s%-20s%-50s\e[27m\n", "Option", "Long option", "Meaning");

    printf("%-10s%-20s%-50s\n", "-h", "--help", "Show this message");
    printf("%-10s%-20s%-50s\n", "-c", "--crash", 
           "Induces a segmentation fault");
}

/* Tests options */
#define IS_TESTS_HELP     TEST_CMD("-h", "--help")
#define IS_TESTS_CRASH    TEST_CMD("-c", "--crash")

int __crash_kserser_daemon()
{
    dev_id_t dev_id;
    op_id_t op_id;
    struct command* cmd;
    
    struct kclient *kcl = __start_client();
    
    if (kcl == NULL) {
        fprintf(stderr, "Connection failed\n");
        return -1;
    }
    
    if ((dev_id = get_device_id(kcl, "TESTS")) < 0) {
        fprintf(stderr, "Unknown device TESTS\n");
        return -1;
    }
    
    if ((op_id = get_op_id(kcl, dev_id, "CRASH")) < 0) {
        fprintf(stderr, "Unknown operation CRASH\n");
        return -1;
    }
    
    if ((cmd = init_command(dev_id)) == NULL) {
        fprintf(stderr, "Cannot allocate crash command\n");
        return -1;
    }
    
    cmd->op_ref = op_id;
    
    if (kclient_send(kcl, cmd) < 0) {
        fprintf(stderr, "Cannot send crash command\n");
        return -1;
    }

    free_command(cmd);
    __stop_client(kcl);
    
    return 0;
}

/**
 * ks_cli_tests - Execute tests
 */
void ks_cli_tests(int argc, char **argv)
{    
    assert(strcmp(argv[0], "tests") == 0);
    
    if (argc < 2) {
        fprintf(stderr, "Available commands:\n\n");
        __tests_usage();
        exit(EXIT_FAILURE);
    }
    
    if (IS_TESTS_CRASH) {    
        if (__crash_kserser_daemon() < 0)
            exit(EXIT_FAILURE);
    }
    else if (IS_TESTS_HELP) {
        __tests_usage();
        exit(EXIT_SUCCESS);
    } else {
        fprintf(stderr, "Invalid set command: %s\n", argv[1]);
        __tests_usage();
        exit(EXIT_FAILURE);
    }
}

#endif

/* 
 *  -------------------------------------------------
 *                      Main
 *  -------------------------------------------------
 */
 
void usage(void)
{
    printf("Usage: kserver COMMAND [arg...]\n\n");
    
    printf("\e[7m%-15s%-50s\e[27m\n", "Command", "Meaning");
    
    printf("%-15s%-50s\n", "help", "Show this message");
    printf("%-15s%-50s\n", "status", "Display the current status of the server");
    printf("%-15s%-50s\n", "kill", "Kill a session (UNSTABLE)");
    printf("%-15s%-50s\n", "host", "Set the connection parameters");
#if KSERVER_CLI_HAS_INIT_TASKS
    printf("%-15s%-50s\n", "init_tasks", "Tasks to be performed at system initialization");
#endif
#if KSERVER_CLI_HAS_TESTS
    printf("%-15s%-50s\n", "tests", "Tests commands. For development purpose");
#endif
    printf("\n");
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Available commands:\n\n");
        usage();
        return 0;
    }

    if (strcmp(argv[1], "status") == 0) {
        ks_cli_status(argc-1, &argv[1]);
    }
    else if (strcmp(argv[1], "host") == 0) {
        ks_cli_host(argc-1, &argv[1]);
    }
    else if (strcmp(argv[1], "kill") == 0) {
        ks_cli_kill(argc-1, &argv[1]);
    }
#if KSERVER_CLI_HAS_INIT_TASKS
    else if (strcmp(argv[1], "init_tasks") == 0) {
        ks_cli_init_tasks(argc-1, &argv[1]);
    }
#endif
#if KSERVER_CLI_HAS_TESTS
    else if (strcmp(argv[1], "tests") == 0) {
        ks_cli_tests(argc-1, &argv[1]);
    }
#endif
    else if (strcmp(argv[1], "--help") == 0) {
        usage();
    } else {
        fprintf(stderr, "Invalid command: %s\n", argv[1]);
        usage();
        exit(EXIT_FAILURE);
    }

    return 0;
}
