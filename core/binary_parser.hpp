/// Utilities to parse binary buffers
///
/// (c) Koheron

#ifndef __BINARY_PARSER_HPP__
#define __BINARY_PARSER_HPP__

#include <cstring>
#include <tuple>

namespace kserver {

// ------------------------
// Type parsing
// ------------------------

template<typename Tp>
Tp parse(char *buff);

template<typename Tp>
constexpr size_t size_of;

// http://stackoverflow.com/questions/17789928/whats-a-proper-way-of-type-punning-a-float-to-an-int-and-vice-versa
template <typename T, typename U>
inline T pseudo_cast(const U &x)
{
    T to = T(0);
    std::memcpy(&to, &x, (sizeof(T) < sizeof(U)) ? sizeof(T) : sizeof(U));
    return to;
}

// -- Specializations

template<> uint16_t parse<uint16_t>(char *buff);
template<> constexpr size_t size_of<uint16_t> = 2;

template<> uint32_t parse<uint32_t>(char *buff);
template<> constexpr size_t size_of<uint32_t> = 4;

template<> float parse<float>(char *buff);
template<> constexpr size_t size_of<float> = 4;

template<> bool parse<bool>(char *buff);
template<> constexpr size_t size_of<bool> = 1;

// ------------------------
// Buffer parsing
// ------------------------

// -- into tuple

// Parse
template<size_t position = 0, typename Tp0, typename... Tp>
inline std::enable_if_t<0 == sizeof...(Tp), std::tuple<Tp0, Tp...>>
parse_buffer(char *buff)
{
    return std::make_tuple(parse<Tp0>(&buff[position]));
}

template<size_t position = 0, typename Tp0, typename... Tp>
inline std::enable_if_t<0 < sizeof...(Tp), std::tuple<Tp0, Tp...>>
parse_buffer(char *buff)
{
    return std::tuple_cat(std::make_tuple(parse<Tp0>(&buff[position])),
                          parse_buffer<position + size_of<Tp0>, Tp...>(buff));
}

// Required buffer size
template<typename Tp0, typename... Tp>
constexpr std::enable_if_t<0 == sizeof...(Tp), size_t>
required_buffer_size()
{
    return size_of<Tp0>;
}

template<typename Tp0, typename... Tp>
constexpr std::enable_if_t<0 < sizeof...(Tp), size_t>
required_buffer_size()
{
    return size_of<Tp0> + required_buffer_size<Tp...>();
}

} // namespace kserver

#endif // __BINARY_PARSER_HPP__