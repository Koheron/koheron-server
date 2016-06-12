/// Serialize and deserialize binary buffers
///
/// (c) Koheron 

#ifndef __BINARY_PARSER_HPP__
#define __BINARY_PARSER_HPP__

#include <cstring>
#include <tuple>

#include "commands.hpp"

namespace kserver {

// http://stackoverflow.com/questions/17789928/whats-a-proper-way-of-type-punning-a-float-to-an-int-and-vice-versa
template <typename T, typename U>
inline T pseudo_cast(const U &x)
{
    T to = T(0);
    std::memcpy(&to, &x, (sizeof(T) < sizeof(U)) ? sizeof(T) : sizeof(U));
    return to;
}

// ------------------------
// Deserializer
// ------------------------

template<typename Tp>
Tp extract(const char *buff);

template<typename Tp>
constexpr size_t size_of;

// -- Definitions

template<>
inline uint16_t extract<uint16_t>(const char *buff)
{
    return (unsigned char)buff[1] + ((unsigned char)buff[0] << 8);
}

template<> constexpr size_t size_of<uint16_t> = 2;

template<>
inline uint32_t extract<uint32_t>(const char *buff)
{
    return (unsigned char)buff[3] + ((unsigned char)buff[2] << 8) 
           + ((unsigned char)buff[1] << 16) + ((unsigned char)buff[0] << 24);
}

template<> constexpr size_t size_of<uint32_t> = 4;

template<>
inline float extract<float>(const char *buff)
{
    static_assert(sizeof(float) == size_of<uint32_t>,
                  "Invalid float size");
    return pseudo_cast<float, uint32_t>(extract<uint32_t>(buff));
}

template<> constexpr size_t size_of<float> = 4;

template<>
inline bool extract<bool>(const char *buff)
{
    return (unsigned char)buff[0] == 1;
}

template<> constexpr size_t size_of<bool> = 1;

namespace detail {

template<size_t position, typename Tp0, typename... Tp>
inline std::enable_if_t<0 == sizeof...(Tp), std::tuple<Tp0, Tp...>>
deserialize(const char *buff)
{
    return std::make_tuple(extract<Tp0>(&buff[position]));
}

template<size_t position, typename Tp0, typename... Tp>
inline std::enable_if_t<0 < sizeof...(Tp), std::tuple<Tp0, Tp...>>
deserialize(const char *buff)
{
    return std::tuple_cat(std::make_tuple(extract<Tp0>(&buff[position])),
                          deserialize<position + size_of<Tp0>, Tp...>(buff));
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

} // namespace detail

template<typename... Tp>
constexpr size_t required_buffer_size()
{
    return detail::required_buffer_size<Tp...>();
}

template<size_t position, size_t len, typename... Tp>
inline std::tuple<Tp...> deserialize(const Buffer<len>& buff)
{
    static_assert(required_buffer_size<Tp...>() <= len - position, 
                  "Buffer size too small");

    return detail::deserialize<position, Tp...>(buff.data);
}

template<size_t position, typename... Tp>
inline std::tuple<Tp...> deserialize(const char *buff)
{
    return detail::deserialize<position, Tp...>(buff);
}

// ------------------------
// Serializer
// ------------------------

} // namespace kserver

#endif // __BINARY_PARSER_HPP__