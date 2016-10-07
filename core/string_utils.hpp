/// (c) Koheron

#ifndef __STRING_UTILS_HPP__
#define __STRING_UTILS_HPP__

#include <cstring>
#include <syslog.h>

namespace kserver {

// -------------------------------------------------------------------------
// Variadic string formating functions accepting parameter packs
// -------------------------------------------------------------------------

// printf
template<typename... Tp>
typename std::enable_if_t< 0 < sizeof...(Tp), void >
printf_pack(const char *fmt, Tp... args) {
    printf(fmt, args...);
}

template<typename... Tp>
typename std::enable_if_t< 0 == sizeof...(Tp), void >
printf_pack(const char *fmt, Tp... args) {
    printf("%s", fmt);
}

// fprintf
template<typename... Tp>
typename std::enable_if_t< 0 < sizeof...(Tp), void >
fprintf_pack(FILE *stream, const char *fmt, Tp... args) {
    fprintf(stream, fmt, args...);
}

template<typename... Tp>
typename std::enable_if_t< 0 == sizeof...(Tp), void >
fprintf_pack(FILE *stream, const char *fmt, Tp... args) {
    fprintf(stream, "%s", fmt);
}

// snprintf
template<typename... Tp>
typename std::enable_if_t< 0 < sizeof...(Tp), int >
snprintf_pack(char *s, size_t n, const char *fmt, Tp... args) {
    return snprintf(s, n, fmt, args...);
}

template<typename... Tp>
typename std::enable_if_t< 0 == sizeof...(Tp), int >
snprintf_pack(char *s, size_t n, const char *fmt, Tp... args) {
    return snprintf(s, n, "%s", fmt);
}

// syslog
template<typename... Tp>
typename std::enable_if_t< 0 < sizeof...(Tp), void >
syslog_pack(int priority, const char *fmt, Tp... args) {
    syslog(priority, fmt, args...);
}

template<typename... Tp>
typename std::enable_if_t< 0 == sizeof...(Tp), void >
syslog_pack(int priority, const char *fmt, Tp... args) {
    syslog(priority, "%s", fmt);
}

// -------------------------------------------------------------------------
// Compile-time string
// -------------------------------------------------------------------------

// http://stackoverflow.com/questions/15858141/conveniently-declaring-compile-time-strings-in-c
class str_const { // constexpr string
  private:
    const char* const p_;
    const std::size_t sz_;

  public:
    template<std::size_t N>
    constexpr str_const(const char(&a)[N]) : // ctor
        p_(a), sz_(N-1) {}

    constexpr char operator[](std::size_t n) { // []
        return n < sz_ ? p_[n] :
        throw std::out_of_range("");
    }

    constexpr std::size_t size() const {return sz_;} // size()

    constexpr const char* data() const {return p_;}
};

} // namespace kserver

#endif // __STRING_UTILS_HPP__