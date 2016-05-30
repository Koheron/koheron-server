/// (c) Koheron

#include "binary_parser.hpp"

namespace kserver {

template<>
uint16_t parse<uint16_t>(const char *buff)
{
    return (unsigned char)buff[1] + ((unsigned char)buff[0] << 8);
}

template<>
uint32_t parse<uint32_t>(const char *buff)
{
    return (unsigned char)buff[3] + ((unsigned char)buff[2] << 8) 
           + ((unsigned char)buff[1] << 16) + ((unsigned char)buff[0] << 24);
}

template<>
float parse<float>(const char *buff)
{
    static_assert(sizeof(float) == size_of<uint32_t>,
                  "Invalid float size");
    return pseudo_cast<float, uint32_t>(parse<uint32_t>(buff));
}

template<>
bool parse<bool>(const char *buff)
{
    return (unsigned char)buff[0] == 1;
}

} // namespace kserver