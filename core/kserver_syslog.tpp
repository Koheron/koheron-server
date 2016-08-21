
#include "kserver.hpp"

namespace kserver {

template<unsigned int severity>
void SysLog::print(const char *message, ...)
{
    va_list argptr, argptr1;
    va_start(argptr, message);
    va_copy(argptr1, argptr);
    notify<severity>(message, argptr);
    emit_error<severity>(message, argptr);
    va_end(argptr1);
    va_end(argptr);
}

template<unsigned int severity>
int SysLog::emit_error(const char *message, va_list argptr)
{
    kserver->pubsub.emit_cstr<PubSub::SYSLOG_CHANNEL, severity>("Error");
    return 0;
}

} // namespace kserver