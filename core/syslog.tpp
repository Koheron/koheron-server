/// (c) Koheron

#include <string>

#include "syslog.hpp"
#include "pubsub.tpp"

namespace kserver {

template<uint16_t channel, uint16_t event, typename... Args>
int SysLog::notify(const char *message, Args&&... args)
{
    // We don't emit if connections are closed
    if (sig_handler.interrupt())
        return 0;

    int ret = kserver::snprintf(fmt_buffer, FMT_BUFF_LEN, message,
                                std::forward<Args>(args)...);

    if (ret < 0) {
        fprintf(stderr, "emit_error: Format error\n");
        return -1;
    }

    if (ret >= FMT_BUFF_LEN) {
        fprintf(stderr, "emit_error: Buffer fmt_buffer overflow\n");
        return -1;
    }

    pubsub.emit_cstr<channel, event>(fmt_buffer);
    return 0;
}

} // namespace kserver