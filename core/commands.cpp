/// Implementation of commands.hpp
///
/// (c) Koheron

#include "commands.hpp"

namespace kserver {

void Command::print(void)
{
    printf("SessID = %u\n", (uint32_t)sess_id);
    printf("Device = %u\n", (uint32_t)device);
    printf("Operation = %u\n", operation);
    printf("Buffer = %s\n", buffer);
    printf("Parsing = %s\n", parsing_err ? "ERR" : "OK");
    printf("Status = %u\n", (uint32_t)status);
}

} // namespace kserver
