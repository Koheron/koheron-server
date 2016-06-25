/// Implementation and specializations of 
/// the class Broadcast in kserver.hpp
///
/// (c) Koheron

#include "kserver.hpp"

#include "kserver_session.hpp"

namespace kserver {

Broadcast::Broadcast(SessionManager& session_manager_)
: session_manager(session_manager_)
, server_chan_subscriptions(0)
{}

int Broadcast::subscribe(uint32_t channel, SessID sid)
{
    if (channel == SERVER_CHANNEL) {
        printf("Session %u subscribed to channel %u\n", sid, channel);
        server_chan_subscriptions.push_back(sid);
    } else {
        return -1;
    }

    return 0;
}

void Broadcast::emit(uint32_t channel)
{
    for (auto const& sid: server_chan_subscriptions) {
    	printf("Broadcasting to session %u on channel %u\n", sid, channel);
        session_manager.GetSession(sid).Send<uint32_t, uint32_t>(
            std::make_tuple(static_cast<uint32_t>(SERVER_CHANNEL), static_cast<uint32_t>(PING))
        );
    }
}

} // namespace kserver