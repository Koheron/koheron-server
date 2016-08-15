/// Implementation and specializations of 
/// the class PubSub in kserver.hpp
///
/// (c) Koheron

#include "kserver.hpp"

#include <algorithm>

namespace kserver {

PubSub::PubSub(SessionManager& session_manager_)
: session_manager(session_manager_)
, subscribers(0)
{}

int PubSub::subscribe(uint32_t channel, SessID sid)
{
    if (channel == SERVER_CHANNEL) {
        printf("Session %u subscribed to channel %u\n", sid, channel);
        subscribers.push_back(sid);
    } else {
        return -1;
    }

    return 0;
}

void PubSub::unsubscribe(SessID sid)
{
    // http://stackoverflow.com/questions/39912/how-do-i-remove-an-item-from-a-stl-vector-with-a-certain-value
    auto it = std::find(subscribers.begin(), subscribers.end(), sid);

    if (it != subscribers.end()) {
        using std::swap;
        swap(*it, subscribers.back());
        subscribers.pop_back();
    }
}

} // namespace kserver