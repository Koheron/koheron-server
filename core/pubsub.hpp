/// PubSub
///
/// (c) Koheron

#ifndef __PUBSUB_HPP__
#define __PUBSUB_HPP__

#include "kserver_defs.hpp"

#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>
#include <type_traits>

namespace kserver {

class SessionManager;

// http://stackoverflow.com/questions/12927951/array-indexing-converting-to-integer-with-scoped-enumeration
template<class channels>
constexpr size_t chan_count() noexcept {
    static_assert(std::is_enum<channels>(), "Not an enum");
    return static_cast<size_t>(channels::channels_count);
}

template<class channels>
struct Subscribers
{
    const std::vector<SessID>& get(uint32_t channel) const {
        return _subscribers[channel];
    }

    int subscribe(uint32_t channel, SessID sid) {
        if (channel >= chan_count<channels>())
            return -1;

        _subscribers[channel].push_back(sid);
        return 0;
    }

    void unsubscribe(SessID sid) {
        for (unsigned int channel = 0; channel < chan_count<channels>(); channel++) {
            // http://stackoverflow.com/questions/39912/how-do-i-remove-an-item-from-a-stl-vector-with-a-certain-value
            auto it = std::find(_subscribers[channel].begin(), _subscribers[channel].end(), sid);

            if (it != _subscribers[channel].end()) {
                using std::swap;
                swap(*it, _subscribers[channel].back());
                _subscribers[channel].pop_back();
            }
        }
    }

  private:
    std::array<std::vector<SessID>, chan_count<channels>()> _subscribers;
};

class PubSub
{
  public:
    PubSub(SessionManager& session_manager_)
    : session_manager(session_manager_)
    {}

    // Session sid subscribes to a channel
    int subscribe(uint32_t channel, SessID sid) {
        return subscribers.subscribe(channel, sid);
    }

    // Must be called when a session is closed
    void unsubscribe(SessID sid) {
        subscribers.unsubscribe(sid);
    }

    // Event message structure
    // |      RESERVED     |      CHANNEL      |       EVENT       |   Arguments
    // |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | ...
    template<uint32_t channel, uint32_t event, typename... Tp>
    void emit(Tp&&... args);

    enum Channels {
        SERVER_CHANNEL,        ///< Server events
#if KSERVER_HAS_DEVMEM
        DEVMEM_CHANNEL,        ///< Devmem events
#endif
        channels_count
    };

    enum ServerChanEvents {
        PING,                   ///< For tests
        NEW_SESSION,            ///< A new session has been started
        DEL_SESSION,            ///< A session has been closed
        server_chan_events_num
    };

  private:
    SessionManager& session_manager;
    Subscribers<Channels> subscribers;
};

} // namespace kserver

#endif // __PUBSUB_HPP__