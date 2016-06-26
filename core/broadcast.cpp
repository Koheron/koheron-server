/// Implementation and specializations of 
/// the class Broadcast in kserver.hpp
///
/// (c) Koheron

#include "kserver.hpp"

#include "kserver_session.hpp"

namespace kserver {

Broadcast::Broadcast(SessionManager& session_manager_)
: session_manager(session_manager_)
, subscribers(0)
{}

int Broadcast::subscribe(uint32_t channel, SessID sid)
{
    if (channel == SERVER_CHANNEL) {
        printf("Session %u subscribed to channel %u\n", sid, channel);
        subscribers.push_back(sid);
    } else {
        return -1;
    }

    return 0;
}

template<uint32_t channel, uint32_t event, typename... Tp>
void Broadcast::emit_event(Tp... args)
{
    for (auto const& sid: subscribers)
        session_manager.GetSession(sid).Send(
            std::tuple_cat(std::make_tuple(0U,   // RESERVED
                                           static_cast<uint32_t>(channel),
                                           static_cast<uint32_t>(event)),
                           std::forward_as_tuple(args...))
        );
}

void Broadcast::emit(uint32_t channel, uint32_t event)
{
    emit_event<SERVER_CHANNEL, PING>();
}

} // namespace kserver