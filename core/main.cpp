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

int main(int argc, char **argv) {
    kserver::KServer server;
    server.run();
    return 0;
}
