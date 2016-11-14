/// Serialize and deserialize binary buffers
///
/// (c) Koheron 

#ifndef __SERIALIZER_DESERIALIZER_HPP__
#define __SERIALIZER_DESERIALIZER_HPP__

#include <cstring>
#include <tuple>
#include <array>

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

template<typename Tp, size_t N=0> constexpr size_t size_of;
template<typename Tp, size_t N> constexpr size_t size_of = size_of<Tp> * N;

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

// int32_t

template<> constexpr size_t size_of<int32_t> = 4;

template<>
inline int32_t extract<int32_t>(const char *buff)
{
    uint32_t tmp = extract<uint32_t>(buff);
    return *reinterpret_cast<int32_t*>(&tmp);
}

template<>
inline void append<int32_t>(unsigned char *buff, int32_t value)
{
    append<uint32_t>(buff, reinterpret_cast<uint32_t&>(value));
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

// int64_t

template<> constexpr size_t size_of<int64_t> = 8;

template<>
inline int64_t extract<int64_t>(const char *buff)
{
    uint64_t tmp = extract<uint64_t>(buff);
    return *reinterpret_cast<int64_t*>(&tmp);
}

template<>
inline void append<int64_t>(unsigned char *buff, int64_t value)
{
    append<uint64_t>(buff, reinterpret_cast<uint64_t&>(value));
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
    template<size_t position, typename... Tp>
    inline std::enable_if_t<0 == sizeof...(Tp), std::tuple<Tp...>>
    deserialize(const char *buff)
    {
        return std::make_tuple();
    }

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
    template<typename... Tp>
    constexpr std::enable_if_t<0 == sizeof...(Tp), size_t>
    required_buffer_size() {
        return 0;
    }

    template<typename Tp0, typename... Tp>
    constexpr std::enable_if_t<0 == sizeof...(Tp), size_t>
    required_buffer_size() {
        return size_of<Tp0>;
    }

    template<typename Tp0, typename... Tp>
    constexpr std::enable_if_t<0 < sizeof...(Tp), size_t>
    required_buffer_size() {
        return size_of<Tp0> + required_buffer_size<Tp...>();
    }
}

template<typename... Tp>
constexpr size_t required_buffer_size()
{
    return detail::required_buffer_size<Tp...>();
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
        using type = typename std::tuple_element_t<I, std::tuple<Tp...>>;
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

template<typename... Tp>
inline std::array<unsigned char, required_buffer_size<Tp...>()>
serialize(Tp... t)
{
    return serialize<Tp...>(std::make_tuple(t...));
}

// ---------------------------
// Commands serializer
// ---------------------------

namespace detail {

    // Scalars

    inline void dump_size_to_buffer(std::vector<unsigned char>& buffer, uint64_t size) {
        buffer.resize(buffer.size() + size_of<uint64_t>);
        append(buffer.data() + buffer.size() - size_of<uint64_t>, size);
    }

    struct ScalarPack {
        static constexpr size_t SCALAR_PACK_LEN = 1024;
        std::array<unsigned char, SCALAR_PACK_LEN> data;
        uint64_t size = 0;

        template<typename T>
        void append(T t) {
            kserver::append<T>(&data[size], t);
            size += size_of<T>;
        }

        void dump_to_buffer(std::vector<unsigned char>& buffer) {
            if (size > 0) {
                dump_size_to_buffer(buffer, size);
                buffer.reserve(buffer.size() + size);
                buffer.insert(buffer.end(), data.data(), data.data() + size);
                size = 0;
            }
        }
    };

    template<typename Tp0, typename... Tp>
    inline std::enable_if_t<0 == sizeof...(Tp) &&
                            std::is_scalar<
                                typename std::remove_reference<Tp0>::type
                            >::value &&
                            !std::is_pointer<
                                typename std::remove_reference<Tp0>::type
                            >::value, void>
    command_serializer(std::vector<unsigned char>& buffer,
                       ScalarPack& scal_pack, Tp0&& t, Tp&&... args)
    {
        scal_pack.append(std::forward<Tp0>(t));
    }

    template <typename Tp0, typename... Tp>
    inline std::enable_if_t<0 < sizeof...(Tp) &&
                            std::is_scalar<
                                typename std::remove_reference<Tp0>::type
                            >::value &&
                            !std::is_pointer<
                                typename std::remove_reference<Tp0>::type
                            >::value, void>
    command_serializer(std::vector<unsigned char>& buffer,
                       ScalarPack& scal_pack, Tp0&& t, Tp&&... args)
    {
        scal_pack.append(std::forward<Tp0>(t));
        command_serializer(buffer, scal_pack, std::forward<Tp>(args)...);
    }

    // Containers (array, vector, string)

    // http://stackoverflow.com/questions/12042824/how-to-write-a-type-trait-is-container-or-is-vector
    template<typename T, typename _ = void>
    struct is_container : std::false_type {};

    template<typename... Ts>
    struct is_container_helper {};

    template<typename T>
    struct is_container<
            T,
            std::conditional_t<
                false,
                is_container_helper<
                    typename T::value_type,
                    typename T::size_type,
                    // typename T::allocator_type, std::array don't have an allocator
                    typename T::iterator,
                    typename T::const_iterator,
                    decltype(std::declval<T>().size()),
                    decltype(std::declval<T>().data()),
                    decltype(std::declval<T>().begin()),
                    decltype(std::declval<T>().end()),
                    decltype(std::declval<T>().cbegin()),
                    decltype(std::declval<T>().cend())
                    >,
                void
                >
            > : public std::true_type {};

    static_assert(is_container<std::vector<float>>::value, "");
    static_assert(is_container<std::array<float, 10>>::value, "");
    static_assert(is_container<std::string>::value, "");
    static_assert(!is_container<float>::value, "");

    template<typename Container>
    inline void dump_container_to_buffer(std::vector<unsigned char>& buffer,
                                         const Container& container)
    {
        auto n_bytes = container.size() * sizeof(typename Container::value_type);
        dump_size_to_buffer(buffer, n_bytes);

        if (n_bytes > 0) {
            const auto bytes = reinterpret_cast<const unsigned char*>(container.data());
            buffer.insert(buffer.end(), bytes, bytes + n_bytes);
        }
    }

    template<typename Tp0, typename... Tp>
    inline std::enable_if_t<0 == sizeof...(Tp) &&
                            is_container<
                                typename std::remove_reference<Tp0>::type
                            >::value, void>
    command_serializer(std::vector<unsigned char>& buffer,
                       ScalarPack& scal_pack, Tp0&& t, Tp&&... args)
    {
        scal_pack.dump_to_buffer(buffer);
        dump_container_to_buffer(buffer, std::forward<Tp0>(t));
    }

    template <typename Tp0, typename... Tp>
    inline std::enable_if_t<0 < sizeof...(Tp) &&
                            is_container<
                                typename std::remove_reference<Tp0>::type
                            >::value, void>
    command_serializer(std::vector<unsigned char>& buffer,
                       ScalarPack& scal_pack, Tp0&& t, Tp&&... args)
    {
        scal_pack.dump_to_buffer(buffer);
        dump_container_to_buffer(buffer, std::forward<Tp0>(t));
        command_serializer(buffer, scal_pack, std::forward<Tp>(args)...);
    }

    // C strings

    // http://stackoverflow.com/questions/8097534/type-trait-for-strings
    template <typename T>
    struct is_c_string : public
    std::integral_constant<bool,
        std::is_same<char*, typename std::remove_reference<T>::type>::value ||
        std::is_same<const char*, typename std::remove_reference<T>::type>::value
    >{};

    static_assert(is_c_string<char*>::value, "");
    static_assert(is_c_string<const char*>::value, "");
    static_assert(!is_c_string<std::string>::value, "");

    template<typename Tp0, typename... Tp>
    inline std::enable_if_t<0 == sizeof...(Tp) && is_c_string<Tp0>::value, void>
    command_serializer(std::vector<unsigned char>& buffer,
                       ScalarPack& scal_pack, Tp0&& t, Tp&&... args)
    {
        scal_pack.dump_to_buffer(buffer);
        dump_container_to_buffer(buffer, std::string(std::forward<Tp0>(t)));
    }

    template <typename Tp0, typename... Tp>
    inline std::enable_if_t<0 < sizeof...(Tp) && is_c_string<Tp0>::value, void>
    command_serializer(std::vector<unsigned char>& buffer,
                       ScalarPack& scal_pack, Tp0&& t, Tp&&... args)
    {
        scal_pack.dump_to_buffer(buffer);
        dump_container_to_buffer(buffer, std::string(std::forward<Tp0>(t)));
        command_serializer(buffer, scal_pack, std::forward<Tp>(args)...);
    }
}

template <typename T>
struct is_std_tuple : std::false_type {};
template <typename... Args>
struct is_std_tuple<std::tuple<Args...>> : std::true_type {};

static_assert(is_std_tuple<std::tuple<uint32_t, float>>::value, "");
static_assert(!is_std_tuple<uint32_t>::value, "");

template<uint16_t class_id, uint16_t func_id, typename Tp0, typename... Args>
inline std::enable_if_t<0 <= sizeof...(Args) &&
                        !is_std_tuple<
                            typename std::remove_reference<Tp0>::type
                        >::value, void>
command_serializer(std::vector<unsigned char>& buffer, Tp0&& arg0, Args&&... args)
{
    const auto& header = serialize(0U, class_id, func_id);
    buffer.resize(header.size());
    std::copy(header.begin(), header.end(), buffer.begin());
    auto scal_pack = detail::ScalarPack{};
    detail::command_serializer(buffer, scal_pack, std::forward<Tp0>(arg0),
                               std::forward<Args>(args)...);
    scal_pack.dump_to_buffer(buffer);
}

template<uint16_t class_id, uint16_t func_id, typename... Args>
inline std::enable_if_t< 0 == sizeof...(Args), void >
command_serializer(std::vector<unsigned char>& buffer, Args&&... args)
{
    const auto& header = serialize(0U, class_id, func_id, 0UL);
    buffer.resize(header.size());
    std::copy(header.begin(), header.end(), buffer.begin());
}

// Tuples are unpacked before serialization

template<uint16_t class_id, uint16_t func_id,
         std::size_t... I, typename... Args>
inline void call_command_serializer(std::vector<unsigned char>& buffer,
                                    std::index_sequence<I...>,
                                    std::tuple<Args...> tup_args)
{
    command_serializer<class_id, func_id>(buffer, std::get<I>(tup_args)...);
}

template<uint16_t class_id, uint16_t func_id, typename... Args>
inline void command_serializer(std::vector<unsigned char>& buffer,
                               std::tuple<Args...> tup_args)
{
    call_command_serializer<class_id, func_id>(buffer,
            std::index_sequence_for<Args...>{}, tup_args);
}

} // namespace kserver

#endif // __SERIALIZER_DESERIALIZER_HPP__