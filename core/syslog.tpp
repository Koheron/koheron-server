/// (c) Koheron

#include <string>

#include "kserver.hpp"
#include "syslog.hpp"

namespace kserver {

template<unsigned int severity, typename... Args>
void SysLog::print(const char *msg, Args&&... args)
{
    static_assert(severity <= syslog_severity_num, "Invalid logging level");

    print_msg<severity>(msg, std::forward<Args>(args)...);
    call_syslog<severity>(msg, std::forward<Args>(args)...);
    emit_error<severity>(msg, std::forward<Args>(args)...);
}

template<unsigned int severity, typename... Args>
int SysLog::__emit_error(const char *message, Args&&... args)
{
    // We don't emit if connections are closed
    if (kserver->sig_handler.interrupt())
        return 0;

    int ret = snprintf(fmt_buffer, FMT_BUFF_LEN, message, std::forward<Args>(args)...);

    if (ret < 0) {
        fprintf(stderr, "emit_error: Format error\n");
        return -1;
    }

    if (ret >= FMT_BUFF_LEN) {
        fprintf(stderr, "emit_error: Buffer fmt_buffer overflow\n");
        return -1;
    }

    kserver->pubsub.emit_cstr<PubSub::SYSLOG_CHANNEL, severity>(fmt_buffer);
    return 0;
}

} // namespace kserver