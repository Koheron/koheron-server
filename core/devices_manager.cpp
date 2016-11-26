/// Implementation of devices_manager.hpp
///
/// (c) Koheron

#include "devices_manager.hpp"
#include "kserver.hpp"
#include "commands.hpp"
#include "syslog.tpp"

#include <devices.hpp>

#if KSERVER_HAS_THREADS
#  include <thread>
#endif

namespace kserver {

DeviceManager::DeviceManager(KServer *kserver_)
: kserver(kserver_)
, dev_cont(kserver->ct)
{
    is_started.fill(false);
}

// Range integer sequence

// http://stackoverflow.com/questions/35625079/offset-for-variadic-template-integer-sequence
template <std::size_t O, std::size_t... Is>
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

template<std::size_t dev>
int DeviceManager::alloc_device() {
    dev_cont.alloc<dev>(); // May fail
    std::get<dev - 2>(device_list)
        = std::make_unique<KDevice<dev>>(kserver, dev_cont.get<dev>());
    std::get<dev - 2>(is_started) = true;
    return 0;
}

template<device_t dev0, device_t... devs>
std::enable_if_t<0 == sizeof...(devs) && 2 <= dev0, int>
DeviceManager::start_impl(device_t dev)
{
    static_assert(dev0 < device_num, "");
    static_assert(dev0 >= 2, "");
    return alloc_device<dev0>();
}

template<device_t dev0, device_t... devs>
std::enable_if_t<0 < sizeof...(devs) && 2 <= dev0, int>
DeviceManager::start_impl(device_t dev)
{
    static_assert(dev0 < device_num, "");
    static_assert(dev0 >= 2, "");

    if (dev == dev0) {
        alloc_device<dev0>();
        return 0;
    } else {
        return start_impl<devs...>(dev);
    }

}

template<device_t... devs>
int DeviceManager::start(device_t dev, std::index_sequence<devs...>)
{
    kserver->syslog.print<SysLog::INFO>(
        "Device Manager: Starting device #%u...\n", dev);
    return start_impl<devs...>(dev);
}

int DeviceManager::init()
{
    if (kserver->ct.init() < 0) {
        kserver->syslog.print<SysLog::CRITICAL>(
                "Context initialization failed\n");
        return -1;
    }

    return 0;
}

template<device_t dev0, device_t... devs>
std::enable_if_t<0 == sizeof...(devs) && 2 <= dev0, int>
DeviceManager::execute_dev_impl(KDeviceAbstract *dev_abs, Command& cmd)
{
    static_assert(dev0 < device_num, "");
    static_assert(dev0 >= 2, "");
    return static_cast<KDevice<dev0>*>(dev_abs)->execute(cmd);
}

template<device_t dev0, device_t... devs>
std::enable_if_t<0 < sizeof...(devs) && 2 <= dev0, int>
DeviceManager::execute_dev_impl(KDeviceAbstract *dev_abs, Command& cmd)
{
    static_assert(dev0 < device_num, "");
    static_assert(dev0 >= 2, "");

    return dev_abs->kind == dev0 ? static_cast<KDevice<dev0>*>(dev_abs)->execute(cmd)
                                 : execute_dev_impl<devs...>(dev_abs, cmd);
}

template<device_t... devs>
int DeviceManager::execute_dev(KDeviceAbstract *dev_abs, Command& cmd,
                               std::index_sequence<devs...>)
{
    static_assert(sizeof...(devs) == device_num - 2, "");
    return execute_dev_impl<devs...>(dev_abs, cmd);
}

int DeviceManager::execute(Command& cmd)
{
    assert(cmd.device < device_num);

    if (cmd.device == 0) {
        return 0;
    } else if (cmd.device == 1) {
        return kserver->execute(cmd);
    } else {
        if (! is_started[cmd.device - 2])
            start(cmd.device, make_index_sequence_in_range<2, device_num>());

        return execute_dev(device_list[cmd.device - 2].get(), cmd,
                           make_index_sequence_in_range<2, device_num>());
    }
}

} // namespace kserver
