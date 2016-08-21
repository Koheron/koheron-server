
#include "kserver.hpp"

namespace kserver {

template<unsigned int severity>
void SysLog::print(const char *message, ...)
{
    va_list argptr, argptr1;
    va_start(argptr, message);
    va_copy(argptr1, argptr);
    notify<severity>(message, argptr);

    // We don't emit if connections are closed
    if(! kserver->sig_handler.Interrupt())
        emit_error<severity>(message, argptr);

    va_end(argptr1);
    va_end(argptr);
}

template<unsigned int severity>
int SysLog::emit_error(const char *message, va_list argptr)
{
    int ret = vsnprintf(fmt_buffer, FMT_BUFF_LEN, message, argptr);

    if (ret < 0) {
        fprintf(stderr, "emit_error: Format error\n");
        return -1;
    }

    if (ret >= FMT_BUFF_LEN) {
        fprintf(stderr, "emit_error: Buffer fmt_buffer overflow\n");
        return -1;
    }

    kserver->pubsub.emit_cstr<PubSub::SYSLOG_CHANNEL, severity>("fmt_buffer");
    return 0;
}

} // namespace kserver