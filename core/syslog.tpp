/// (c) Koheron

#ifndef __KSERVER_SYSLOG_TPP__
#define __KSERVER_SYSLOG_TPP__

#include <string>

#include "syslog.hpp"
#include "pubsub.tpp"

namespace kserver {

template<uint16_t channel, uint16_t event, typename... Args>
inline int SysLog::notify(Args&&... args) {
    return pubsub.emit<channel, event>(std::forward<Args>(args)...);
}

} // namespace kserver

# endif // __KSERVER_SYSLOG_TPP__