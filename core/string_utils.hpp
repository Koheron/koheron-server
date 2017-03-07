/// (c) Koheron

#ifndef __STRING_UTILS_HPP__
#define __STRING_UTILS_HPP__

#include <cstring>
#include <string>
#include <syslog.h>

namespace kserver {

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

    std::string to_string() const {return std::string(p_);}
};

template<class T, class... Tail, class Elem = typename std::decay<T>::type>
constexpr std::array<Elem,1+sizeof...(Tail)> make_array(T&& head, Tail&&... values)
{
  return { std::forward<T>(head), std::forward<Tail>(values)... };
}

} // namespace kserver

#endif // __STRING_UTILS_HPP__