/// Utilities to parse binary buffers
///
/// (c) Koheron

#ifndef __BINARY_PARSER_HPP__
#define __BINARY_PARSER_HPP__

namespace kserver {

inline uint16_t parse_u16(char *buff)
{
    return buff[1] + (buff[0] << 8);
}

inline uint32_t parse_u32(char *buff)
{
    return buff[3] + (buff[2] << 8) + (buff[1] << 16) + (buff[0] << 24);
}

} // namespace kserver

#endif // __BINARY_PARSER_HPP__