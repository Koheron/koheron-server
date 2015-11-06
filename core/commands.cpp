/// @file commands.cpp
///
/// @brief Implementation of commands.hpp
///
/// @author Thomas Vanderbruggen <thomas@koheron.com>
/// @date 11/01/2015
///
/// (c) Koheron 2014-2015

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
