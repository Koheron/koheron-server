/// Utilities to parse binary buffers
///
/// (c) Koheron

#ifndef __BINARY_PARSER_HPP__
#define __BINARY_PARSER_HPP__

#include <tuple>

namespace kserver {

template<typename Tp>
Tp parse(char *buff);

// Specializations
template<>
uint16_t parse<uint16_t>(char *buff)
{
    return buff[1] + (buff[0] << 8);
}

template<>
uint32_t parse<uint32_t>(char *buff)
{
    return buff[3] + (buff[2] << 8) + (buff[1] << 16) + (buff[0] << 24);
}

template<>
float parse<float>(char *buff)
{
	return static_cast<float>(parse<uint32_t>(buff));
}

// Bool are encode on 1 byte
template<>
bool parse<bool>(char *buff)
{
	return buff[0] == 0;
}

/*template<typename... Tp>
std::tuple<Tp...>& parse_buffer(char *buff)
{

}*/

} // namespace kserver

#endif // __BINARY_PARSER_HPP__