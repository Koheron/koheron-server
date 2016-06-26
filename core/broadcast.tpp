/// Broadcasting
///
/// (c) Koheron

#include "kserver_session.hpp"

namespace kserver {

template<uint32_t channel, uint32_t event, typename... Tp>
void Broadcast::emit(Tp&&... args)
{
    for (auto const& sid: subscribers)
        session_manager.GetSession(sid).Send(
            std::make_tuple(0U,   // RESERVED
                            channel,
                            event,
                            args...)
        );
}

// template<> auto emit_event<SERVER_CHANNEL, PING>();
// template<> auto emit_event<SERVER_CHANNEL, NEW_SESSION, sock_type>(int sock_type);

} // namespace kserver
