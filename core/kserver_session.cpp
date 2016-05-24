/// (c) Koheron

#include "kserver_session.hpp"

namespace kserver {

const uint32_t* SessionAbstract::RcvHandshake(uint32_t buff_size)
{
    switch (this->kind) {
      case TCP:
        return TCP_SESSION->RcvHandshake(buff_size);
      case UNIX:
        return UNIX_SESSION->RcvHandshake(buff_size);
      case WEBSOCK:
        return WEB_SESSION->RcvHandshake(buff_size);
      default: assert(false);
    }

    return nullptr;
}

int SessionAbstract::SendCstr(const char *string)
{
    switch (this->kind) {
      case TCP:
        return TCP_SESSION->SendCstr(string);
      case UNIX:
        return UNIX_SESSION->SendCstr(string);
      case WEBSOCK:
        return WEB_SESSION->SendCstr(string);
      default: assert(false);
    }

    return -1;
}

} // namespace kserver