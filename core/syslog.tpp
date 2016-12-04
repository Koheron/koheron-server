/// (c) Koheron

#ifndef __KSERVER_SYSLOG_TPP__
#define __KSERVER_SYSLOG_TPP__

#include <string>

#include "syslog.hpp"
#include "pubsub.tpp"

namespace kserver {

template<uint16_t channel, uint16_t event, typename... Args>
inline int SysLog::notify(const char *message, Args&&... args)
{
    // We don't emit if connections are closed
    if (sig_handler.interrupt())
        return 0;

    return pubsub.emit_cstr<channel, event>(message, std::forward<Args>(args)...);
}

} // namespace kserver

# endif // __KSERVER_SYSLOG_TPP__