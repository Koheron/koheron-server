/// (c) Koheron

#include "kserver_session.hpp"

namespace kserver {

const uint32_t* SessionAbstract::RcvHandshake(uint32_t buff_size)
{
	SWITCH_SOCK_TYPE(RcvHandshake(buff_size))
    return nullptr;
}

int SessionAbstract::SendCstr(const char *string)
{
	SWITCH_SOCK_TYPE(SendCstr(string))
    return -1;
}

} // namespace kserver