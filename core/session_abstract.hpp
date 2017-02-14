/// (c) Koheron

#ifndef __SESSION_ABSTRACT_HPP__
#define __SESSION_ABSTRACT_HPP__

#include "commands.hpp"

namespace kserver {

class SessionAbstract
{
  public:
    explicit SessionAbstract(int sock_type_)
    : kind(sock_type_) {}

    template<typename... Tp> std::tuple<int, Tp...> deserialize(Command& cmd);
    template<typename Tp> int recv(Tp& container, Command& cmd);
    template<uint16_t class_id, uint16_t func_id, typename... Args> int send(Args&&... args);

    int kind;
};

} // namespace kserver

#endif // __SESSION_ABSTRACT_HPP__