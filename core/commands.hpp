/// Commands received by the server
///
/// (c) Koheron

#ifndef __COMMANDS_HPP__
#define __COMMANDS_HPP__

#include "session_manager.hpp"
#include "dev_definitions.hpp"

namespace kserver {

template<size_t len>
struct Buffer
{
    char data[len];
    constexpr size_t size() const {return len;}
    void set() {bzero(data, len);}
};

struct Command
{
    SessID sess_id = -1;                    ///< ID of the session emitting the command  
    device_t device = NO_DEVICE;            ///< The device to control
    uint32_t operation = -1;                ///< Operation ID
    uint32_t payload_size;
    Buffer<CMD_PAYLOAD_BUFFER_LEN> buffer;  ///< data buffer TODO: rename payload
    
    void print() const;
};

} // namespace kserver

#endif // __COMMANDS_HPP__

