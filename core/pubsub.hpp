/// PubSub
///
/// (c) Koheron

#ifndef __PUBSUB_HPP__
#define __PUBSUB_HPP__

#include "kserver_defs.hpp"
#include "commands.hpp"

#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>
#include <type_traits>

#if KSERVER_HAS_THREADS
#  include <thread>
#  include <mutex>
#endif

namespace kserver {

class SessionManager;

#define EMIT_BUFF_SIZE 4096

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
        for (auto& s : _subscribers) {
            // http://stackoverflow.com/questions/39912/how-do-i-remove-an-item-from-a-stl-vector-with-a-certain-value
            auto it = std::find(s.begin(), s.end(), sid);

            if (it != s.end()) {
                using std::swap;
                swap(*it, s.back());
                s.pop_back();
            }
        }
    }

    int count(uint32_t channel) {
        return _subscribers[channel].size();
    }

  private:
    std::array<std::vector<SessID>, chan_count<channels>()> _subscribers;
};

class PubSub
{
  public:
    PubSub(SessionManager& session_manager_)
    : session_manager(session_manager_)
    , emit_buffer(0)
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

    template<uint32_t channel, uint32_t event>
    void emit_cstr(const char *str);

    enum Channels {
        SERVER_CHANNEL,        ///< Server events
        SYSLOG_CHANNEL,        ///< Syslog events
        channels_count
    };

    enum ServerChanEvents {
        PING,                   ///< For tests
        server_chan_events_num
    };

  private:
    SessionManager& session_manager;
    Subscribers<Channels> subscribers;
    std::vector<unsigned char> emit_buffer;

#if KSERVER_HAS_THREADS
    std::mutex mutex;
#endif
};

} // namespace kserver

#endif // __PUBSUB_HPP__