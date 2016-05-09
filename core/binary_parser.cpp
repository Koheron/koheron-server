/// Implementation of binary_parser.hpp
///
/// (c) Koheron

#include "binary_parser.hpp"

namespace kserver {

template<>
uint16_t parse<uint16_t>(const char *buff)
{
    return buff[1] + (buff[0] << 8);
}

template<>
uint32_t parse<uint32_t>(const char *buff)
{
    return buff[3] + (buff[2] << 8) + (buff[1] << 16) + (buff[0] << 24);
}

template<>
float parse<float>(const char *buff)
{
    static_assert(sizeof(float) == size_of<uint32_t>,
                  "Invalid float size");
    return pseudo_cast<float>(parse<uint32_t>(buff));
}

template<>
bool parse<bool>(const char *buff)
{
    return buff[0] == 1;
}

} // namespace kserver