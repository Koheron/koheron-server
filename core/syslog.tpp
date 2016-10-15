
#include "syslog.hpp"

#include <string>

#include "kserver.hpp"

#include "syslog.hpp"

namespace kserver {

template<unsigned int severity, typename... Tp>
int SysLog::__emit_error(const std::string& message, Tp... args)
{
    // We don't emit if connections are closed
    if (kserver->sig_handler.Interrupt())
        return 0;

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