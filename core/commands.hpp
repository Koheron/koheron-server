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
    size_t position = 0; // Current position in the buffer

    constexpr size_t size() const {return len;}
    void set() {bzero(data, len);}
};

struct Command
{
    Command()
    {
        header.position = HEADER_START;
    }

    enum Header : uint32_t {
        HEADER_SIZE = 12,
        HEADER_START = 4  // First 4 bytes are reserved
    };

    SessID sess_id = -1;                    ///< ID of the session emitting the command  
    device_t device = NO_DEVICE;            ///< The device to control
    uint32_t operation = -1;                ///< Operation ID
    size_t payload_size;

    Buffer<HEADER_SIZE> header;             ///< Raw data header
    Buffer<CMD_PAYLOAD_BUFFER_LEN> payload;
    
    void print() const {
        printf("SessID = %u\n", (uint32_t)sess_id);
        printf("Device = %u\n", (uint32_t)device);
        printf("Operation = %u\n", operation);
        printf("Payload = %s\n", payload.data);
    }
};

} // namespace kserver

#endif // __COMMANDS_HPP__

