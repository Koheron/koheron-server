/// Main file for koheron-server
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

#include <sys/types.h>
#include <sys/stat.h>
#include "core/kserver.hpp"
#include "core/config.hpp"

// Run koheron-server as a daemon process
// From http://www.thegeekstuff.com/2012/02/c-daemon-process/
void daemonize()
{
    pid_t process_id = fork();

    if (process_id < 0) {
        fprintf(stderr, "koheron-server: fork failed!\n");
        exit(EXIT_FAILURE);
    }

    if (process_id > 0)
        exit(EXIT_SUCCESS);

    umask(0);

    pid_t sid = setsid();

    if (sid < 0)
        exit(EXIT_FAILURE);

    if (chdir("/") < 0) {
        fprintf(stderr, "koheron-server: Cannot change current directory to root\n");
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

// -----------------------------
// Main
// -----------------------------

int main(int argc, char **argv)
{
    // Load config and options
    auto config = std::make_shared<kserver::KServerConfig>();

    if (config->daemon)
        daemonize();

    kserver::KServer server(config);
    server.run();

    exit(EXIT_SUCCESS);

    return 0;
}
