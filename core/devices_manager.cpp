/// Implementation of devices_manager.hpp
///
/// (c) Koheron

#include "devices_manager.hpp"
#include "kserver.hpp"
#include "commands.hpp"
#include "syslog.tpp"

#if KSERVER_HAS_THREADS
#  include <thread>
#endif

namespace kserver {

DeviceManager::DeviceManager(KServer *kserver_)
: kserver(kserver_)
, ct()
{}

// X Macro: Start new device
#define EXPAND_AS_START_DEVICE(num, name)            \
    device_list[num - 2] = std::make_unique<name>(kserver, ct);

int DeviceManager::init()
{
    if (ct.init() < 0) {
        kserver->syslog.print<SysLog::CRITICAL>(
                "Context initialization failed\n");
        return -1;
    }

    DEVICES_TABLE(EXPAND_AS_START_DEVICE) // Start all devices
    return 0;
}

template<device_t dev0, device_t... devs>
std::enable_if_t<0 == sizeof...(devs) && 2 <= dev0, int>
execute_dev_impl(KDeviceAbstract *dev_abs, Command& cmd) {
    static_assert(dev0 < device_num, "");
    static_assert(dev0 >= 2, "");
    return static_cast<KDevice<dev0>*>(dev_abs)->execute(cmd);
}

template<device_t dev0, device_t... devs>
std::enable_if_t<0 < sizeof...(devs) && 2 <= dev0, int>
execute_dev_impl(KDeviceAbstract *dev_abs, Command& cmd) {
    static_assert(dev0 < device_num, "");
    static_assert(dev0 >= 2, "");

    return dev_abs->kind == dev0 ? static_cast<KDevice<dev0>*>(dev_abs)->execute(cmd)
                                 : execute_dev_impl<devs...>(dev_abs, cmd);
}

template<device_t... devs>
int execute_dev(KDeviceAbstract *dev_abs, Command& cmd,
                std::index_sequence<devs...>) {
    static_assert(sizeof...(devs) == device_num - 2, "");
    return execute_dev_impl<devs...>(dev_abs, cmd);
}

// http://stackoverflow.com/questions/35625079/offset-for-variadic-template-integer-sequence
template <std::size_t O, std::size_t ... Is>
std::index_sequence<(O + Is)...> add_offset(std::index_sequence<Is...>)
{ return {}; }

template <std::size_t O, std::size_t N>
auto make_index_sequence_with_offset() {
    return add_offset<O>(std::make_index_sequence<N>{});
}

template <std::size_t First, std::size_t Last>
auto make_index_sequence_in_range() {
    return make_index_sequence_with_offset<First, Last - First>();
}

int DeviceManager::execute(Command& cmd)
{
    assert(cmd.device < device_num);

    if (cmd.device == 0)
        return 0;
    else if (cmd.device == 1)
        return kserver->execute(cmd);
    else
        return execute_dev(device_list[cmd.device - 2].get(), cmd,
                           make_index_sequence_in_range<2, device_num>());
}

} // namespace kserver
