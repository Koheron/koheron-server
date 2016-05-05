/// Implementation of binary_parser.hpp
///
/// (c) Koheron

#include "binary_parser.hpp"

namespace kserver {

template<>
uint16_t parse<uint16_t>(char *buff)
{
    return buff[1] + (buff[0] << 8);
}

// uint32_t

template<>
uint32_t parse<uint32_t>(char *buff)
{
    return buff[3] + (buff[2] << 8) + (buff[1] << 16) + (buff[0] << 24);
}

// float

template<>
float parse<float>(char *buff)
{
    return static_cast<float>(parse<uint32_t>(buff));
}

// bool
// Coded on 1 byte

template<>
bool parse<bool>(char *buff)
{
    return buff[0] == 0;
}

} // namespace kserver