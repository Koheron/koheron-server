/// (c) Koheron

#ifndef __PUBSUB_TPP__
#define __PUBSUB_TPP__

#include "pubsub.hpp"
#include "kserver_session.hpp"

#if KSERVER_HAS_THREADS
#  include <thread>
#  include <mutex>
#endif

namespace kserver {

template<uint16_t channel, uint16_t event, typename... Tp>
inline void PubSub::emit(Tp&&... args)
{
    static_assert(channel < channels_count, "Invalid channel");

    for (auto const& sid : subscribers.get<channel>())
        session_manager.get_session(sid).template send<channel, event>(
            std::make_tuple(std::forward<Tp>(args)...)
        );
}

template<uint16_t channel, uint16_t event, typename... Args>
inline int PubSub::emit_cstr(const char *str, Args&&... args)
{
    static_assert(channel < channels_count, "Invalid channel");

    // We don't emit if connections are closed
    if (sig_handler.interrupt())
        return 0;

    int ret = kserver::snprintf(fmt_buffer, FMT_BUFF_LEN, str,
                                std::forward<Args>(args)...);

    if (unlikely(ret < 0)) {
        fprintf(stderr, "emit_error: Format error\n");
        return -1;
    }

    if (unlikely(ret >= FMT_BUFF_LEN)) {
        fprintf(stderr, "emit_error: Buffer fmt_buffer overflow\n");
        return -1;
    }

    if (subscribers.count<channel>() > 0) {
        for (auto const& sid : subscribers.get<channel>())
            session_manager.get_session(sid).template send<channel, event>(fmt_buffer);
    }

    return 0;
}

} // namespace kserver

#endif // __PUBSUB_TPP__
