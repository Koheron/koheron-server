/// Main file for koheron-server
///
/// (c) Koheron

#include "core/kserver.hpp"

int main(int argc, char **argv) {
    kserver::KServer server;
    server.run();
    return 0;
}
