/// Serialize and deserialize binary buffers
///
/// (c) Koheron 

#ifndef __SERIALIZER_DESERIALIZER_HPP__
#define __SERIALIZER_DESERIALIZER_HPP__

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
// Definitions
// ------------------------

template<typename Tp> constexpr size_t size_of;
template<typename Tp> Tp extract(const char *buff);                // Deserialization
template<typename Tp> void append(unsigned char *buff, Tp value);  // Serialization

// uint8_t

template<> constexpr size_t size_of<uint8_t> = 1;

template<>
inline uint8_t extract<uint8_t>(const char *buff)
{
    return (unsigned char)buff[0];
}

template<>
inline void append<uint8_t>(unsigned char *buff, uint8_t value)
{
    buff[0] = value;
}

// int8_t

template<> constexpr size_t size_of<int8_t> = 1;

template<>
inline int8_t extract<int8_t>(const char *buff)
{
    return buff[0];
}

template<>
inline void append<int8_t>(unsigned char *buff, int8_t value)
{
    buff[0] = reinterpret_cast<uint8_t&>(value);
}

// uint16_t

template<> constexpr size_t size_of<uint16_t> = 2;

template<>
inline uint16_t extract<uint16_t>(const char *buff)
{
    return (unsigned char)buff[1] + ((unsigned char)buff[0] << 8);
}

template<>
inline void append<uint16_t>(unsigned char *buff, uint16_t value)
{
    buff[0] = (value >> 8) & 0xff;
    buff[1] = value & 0xff;
}

// int16_t

template<> constexpr size_t size_of<int16_t> = 2;

template<>
inline int16_t extract<int16_t>(const char *buff)
{
    uint16_t tmp = extract<uint16_t>(buff);
    return *reinterpret_cast<int16_t*>(&tmp);
}

template<>
inline void append<int16_t>(unsigned char *buff, int16_t value)
{
    append<uint16_t>(buff, reinterpret_cast<uint16_t&>(value));
}

// uint32_t

template<> constexpr size_t size_of<uint32_t> = 4;

template<>
inline uint32_t extract<uint32_t>(const char *buff)
{
    return (unsigned char)buff[3] + ((unsigned char)buff[2] << 8) 
           + ((unsigned char)buff[1] << 16) + ((unsigned char)buff[0] << 24);
}

template<>
inline void append<uint32_t>(unsigned char *buff, uint32_t value)
{
    buff[0] = (value >> 24) & 0xff;
    buff[1] = (value >> 16) & 0xff;
    buff[2] = (value >>  8) & 0xff;
    buff[3] = value & 0xff;
}

// uint64_t

template<> constexpr size_t size_of<uint64_t> = 8;

template<>
inline uint64_t extract<uint64_t>(const char *buff)
{
    uint32_t u1 = extract<uint32_t>(buff);
    uint32_t u2 = extract<uint32_t>(buff + size_of<uint32_t>);
    return static_cast<uint64_t>(u1) + (static_cast<uint64_t>(u2) << 32);
}

template<>
inline void append<uint64_t>(unsigned char *buff, uint64_t value)
{
    append<uint32_t>(buff, (value >> 32));
    append<uint32_t>(buff + size_of<uint32_t>, value);
}

// float

template<> constexpr size_t size_of<float> = size_of<uint32_t>;
static_assert(sizeof(float) == size_of<float>, "Invalid float size");

template<>
inline float extract<float>(const char *buff)
{
    return pseudo_cast<float, uint32_t>(extract<uint32_t>(buff));
}

template<>
inline void append<float>(unsigned char *buff, float value)
{
    append<uint32_t>(buff, pseudo_cast<uint32_t, float>(value));
}

// double

template<> constexpr size_t size_of<double> = size_of<uint64_t>;
static_assert(sizeof(double) == size_of<double>, "Invalid double size");

template<>
inline double extract<double>(const char *buff)
{
    return pseudo_cast<double, uint64_t>(extract<uint64_t>(buff));
}

template<>
inline void append<double>(unsigned char *buff, double value)
{
    append<uint64_t>(buff, pseudo_cast<uint64_t, double>(value));
}

// bool

template<> constexpr size_t size_of<bool> = 1;

template<>
inline bool extract<bool>(const char *buff)
{
    return (unsigned char)buff[0] == 1;
}

template<>
inline void append<bool>(unsigned char *buff, bool value)
{
    value ? buff[0] = 1 : buff[0] = 0;
}

// ------------------------
// Deserializer
// ------------------------

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

namespace detail {

template<size_t buff_pos, size_t I, typename... Tp>
inline std::enable_if_t<I == sizeof...(Tp), void>
serialize(const std::tuple<Tp...>& t, unsigned char *buff)
{}

template<size_t buff_pos, size_t I, typename... Tp>
inline std::enable_if_t<I < sizeof...(Tp), void>
serialize(const std::tuple<Tp...>& t, unsigned char *buff)
{
    using type = typename std::tuple_element<I, std::tuple<Tp...>>::type;
    append<type>(&buff[buff_pos], std::get<I>(t));
    serialize<buff_pos + size_of<type>, I + 1, Tp...>(t, &buff[0]);
}

}

template<typename... Tp>
inline std::array<unsigned char, required_buffer_size<Tp...>()>
serialize(const std::tuple<Tp...>& t)
{
    std::array<unsigned char, required_buffer_size<Tp...>()> arr;
    detail::serialize<0, 0, Tp...>(t, arr.data());
    return arr;
}

} // namespace kserver

#endif // __SERIALIZER_DESERIALIZER_HPP__