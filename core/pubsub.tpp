/// (c) Koheron

#include "pubsub.hpp"
#include "kserver_session.hpp"

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
    static_assert(channel < channels_count, "Invalid channel");
    uint32_t len = strlen(str);
    assert(len + 4 * sizeof(uint32_t) <= EMIT_BUFF_SIZE);
    auto tup = std::make_tuple(0U, channel, event, len);
    auto array = serialize(tup);
    memcpy(emit_buffer.data, array.data(), array.size());
    memcpy(emit_buffer.data + array.size(), str, len);

    for (auto const& sid : subscribers.get(channel))
        session_manager.get_session(sid).send_array(emit_buffer.data, len + 4 * sizeof(uint32_t));
}

} // namespace kserver
