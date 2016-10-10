
#include "syslog.hpp"

#include <string>

#include "kserver.hpp"

namespace kserver {

template<unsigned int severity, typename... Tp>
void SysLog::print(const char *message, Tp... args)
{
    auto msg = std::string(message);
    notify<severity>(msg, args...);

    // We don't emit if connections are closed
    if (! kserver->sig_handler.Interrupt())
        emit_error<severity>(msg, args...);
}

template<unsigned int severity, typename... Tp>
int SysLog::emit_error(const std::string& message, Tp... args)
{
    int ret = snprintf(fmt_buffer, FMT_BUFF_LEN, message, args...);

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