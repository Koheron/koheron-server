/// (c) Koheron

#include "pubsub.hpp"
#include "kserver_session.hpp"

#if KSERVER_HAS_THREADS
#  include <thread>
#  include <mutex>
#endif

namespace kserver {

template<uint32_t channel, uint32_t event, typename... Tp>
void PubSub::emit(Tp&&... args)
{
    static_assert(channel < channels_count, "Invalid channel");

    for (auto const& sid : subscribers.get(channel))
        session_manager.get_session(sid).send(
            std::make_tuple(0U,   // RESERVED
                            channel,
                            event,
                            args...)
        );
}

template<uint32_t channel, uint32_t event>
void PubSub::emit_cstr(const char *str)
{
    // Need a mutex: emit_buffer is shared memory
#if KSERVER_HAS_THREADS
    std::lock_guard<std::mutex> lock(mutex);
#endif

    static_assert(channel < channels_count, "Invalid channel");

    auto string = std::string(str);
    uint32_t len = string.size() + 1; // Including '\0'
    auto array = serialize(std::make_tuple(0U, channel, event, len));
    emit_buffer.resize(0);
    emit_buffer.insert(emit_buffer.end(), array.begin(), array.end());
    emit_buffer.insert(emit_buffer.end(), string.begin(), string.end());
    emit_buffer.push_back('\0');

    for (auto const& sid : subscribers.get(channel))
        session_manager.get_session(sid).send(emit_buffer);
}

} // namespace kserver
