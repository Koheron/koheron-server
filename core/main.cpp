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

int main(int argc, char **argv)
{
    // Load config and options
    auto config = std::make_shared<kserver::KServerConfig>();

    printf("Start server \n");

    kserver::KServer server(config);

    printf("Config loaded \n");

    server.run();

    return 0;
}
