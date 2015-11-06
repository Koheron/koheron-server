/// @file tuple_utils.hpp
///
/// @brief Utilities for tuple manipulations
///
/// @author Thomas Vanderbruggen <thomas@koheron.com>
/// @date 06/10/2015
///
/// (c) Koheron 2014-2015

#ifndef __TUPLE_UTILS_HPP__
#define __TUPLE_UTILS_HPP__

#include <tuple>
#include <utility> 
#include <iostream>
#include <sstream>
#include <string>
#include <typeinfo>

namespace kserver {

// Compile time recursion
// http://stackoverflow.com/questions/1198260/iterate-over-tuple

// Initialize recursion
template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
stringify_tuple(const std::tuple<Tp...>& t, std::stringstream& ss)
{}

template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I < sizeof...(Tp), void>::type
stringify_tuple(const std::tuple<Tp...>& t, std::stringstream& ss)
{
    using type = typename std::tuple_element<I, std::tuple<Tp...>>::type;
    ss << typeid(type).name() << "@" << std::get<I>(t) << ":";
    stringify_tuple<I + 1, Tp...>(t, ss);
}

} // namespace kserver

#endif // __TUPLE_UTILS_HPP__
