/// Main file for Server
///
/// (c) Koheron

#include "core/kserver_defs.hpp"

#include <iostream>
#include <cstdlib>
#include <utility>

extern "C" {
  #include <unistd.h>
  #include <getopt.h>
}

#if KSERVER_IS_DAEMON
  #include <sys/types.h>
  #include <sys/stat.h>
#endif

#include "core/kserver.hpp"
#include "core/config.hpp"

// -----------------------------
// Commands
// -----------------------------

constexpr char opt_str[] = "hc:vlp:d:w";

static struct option lopts[] = {
    {"help"     ,   0, 	0, 	'h'},
    {"config"   ,   1, 	0, 	'c'},
    {"verbose"  ,   0, 	0, 	'v'}
};

void usage(void)
{
    printf("Usage: kserver [Option]\n\n");
    printf("Option \t\t Long option \t\t Meaning\n");
    printf("------------------------------------------"
           "--------------------------------------\n");

    printf("-h \t\t --help \t\t Show this message\n");
    printf("-c \t\t --config \t\t Load a configuration file\n");
    printf("-v \t\t --verbose \t\t Display send and received data\n");
}

void parse_options(int argc, char **argv, 
                   std::shared_ptr<kserver::KServerConfig> const& config)
{
    int cmdx = 0;
    int opt_ch;

    while ((opt_ch = getopt_long(argc, argv, opt_str,
                                lopts, &cmdx)) != EOF) {
        switch (opt_ch) {
          case 'v':
            config->verbose = 1;
            break;
          case 'c':
            if (config->load_file(optarg) < 0) {
                fprintf(stderr, "Cannot load configuration file %s\n", optarg);
                exit (EXIT_FAILURE);
            }
            break;
          case 'h':
            usage();
            exit (EXIT_SUCCESS);
          default:
            usage();
            exit (EXIT_FAILURE);
        }
    }
}

// -----------------------------
// Daemon
// -----------------------------

#if KSERVER_IS_DAEMON

// Run tcp-server as a daemon process
// From http://www.thegeekstuff.com/2012/02/c-daemon-process/
void daemonize()
{
    pid_t process_id = fork();

    if (process_id < 0) {
        fprintf(stderr, "tcp-server: fork failed!\n");
        exit(EXIT_FAILURE);
    }

    if (process_id > 0)
        exit(EXIT_SUCCESS);

    umask(0);

    pid_t sid = setsid();

    if (sid < 0)
        exit(EXIT_FAILURE);

    if (chdir("/") < 0) {
        fprintf(stderr, "tcp-server: Cannot change current directory to root\n");
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

#endif // KSERVER_IS_DAEMON

// -----------------------------
// Main
// -----------------------------

int main(int argc, char **argv)
{
    // Load config and options
    auto config = std::make_shared<kserver::KServerConfig>();
    parse_options(argc, argv, config);

#if KSERVER_IS_DAEMON
    if (config->daemon)
        daemonize();
#endif

    kserver::KServer server(config);
    server.Run();

    // FIXME Not a clean way to terminate
    // But else the program doesn't close
    // after a SIGINT.
    // It is probably because some threads
    // are still running ...
    exit(EXIT_SUCCESS);

    return 0;
}
