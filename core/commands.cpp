/// Implementation of commands.hpp
///
/// (c) Koheron

#include "commands.hpp"

namespace kserver {

void Command::print() const
{
    printf("SessID = %u\n", (uint32_t)sess_id);
    printf("Device = %u\n", (uint32_t)device);
    printf("Operation = %u\n", operation);
    printf("Buffer = %s\n", buffer.data);
}

} // namespace kserver
