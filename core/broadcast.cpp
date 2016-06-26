/// Implementation and specializations of 
/// the class Broadcast in kserver.hpp
///
/// (c) Koheron

#include "kserver.hpp"

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

} // namespace kserver