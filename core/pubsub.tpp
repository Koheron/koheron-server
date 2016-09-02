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

    auto string = std::string(str);
    uint32_t len = string.size() + 1; // Including '\0'
    auto array = serialize(std::make_tuple(0U, channel, event, len));
    std::vector<char> data(array.begin(), array.end());
    std::copy(string.begin(), string.end(), std::back_inserter(data));
    data.push_back('\0');

    for (auto const& sid : subscribers.get(channel))
        session_manager.get_session(sid).send(data);
}

} // namespace kserver
