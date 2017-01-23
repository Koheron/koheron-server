/// Implementation of devices_manager.hpp
///
/// (c) Koheron

#include "syslog.tpp"

#include "devices_manager.hpp"
#include "kserver.hpp"
#include "commands.hpp"
// #include "syslog.tpp"
#include "meta_utils.hpp"
#include <ks_devices.hpp>

namespace kserver {

//----------------------------------------------------------------------------
// Device container
//----------------------------------------------------------------------------

template<device_id dev>
int DevicesContainer::alloc() {
    if (std::get<dev - 2>(is_started))
        return 0;

    if (std::get<dev - 2>(is_starting)) {
        syslog.print<CRITICAL>(
            "Circular dependency detected while initializing device [%u] %s\n",
            dev, std::get<dev>(devices_names).data());

        return -1;
    }

    std::get<dev - 2>(is_starting) = true;
    std::get<dev - 2>(devtup) = std::make_unique<device_t<dev>>(ctx);
    std::get<dev - 2>(is_starting) = false;
    std::get<dev - 2>(is_started) = true;
    return 0;
}

//----------------------------------------------------------------------------
// Device manager
//----------------------------------------------------------------------------

DeviceManager::DeviceManager(KServer *kserver_)
: kserver(kserver_)
, dev_cont(ctx, kserver->syslog)
{
    ctx.set_device_manager(this);
    ctx.set_syslog(&kserver->syslog);
    is_started.fill(false);
}

template<std::size_t dev>
void DeviceManager::alloc_device()
{
#if KSERVER_HAS_THREADS
    std::lock_guard<std::recursive_mutex> lock(mutex);
#endif

    if (std::get<dev - 2>(is_started))
        return;

    kserver->syslog.print<INFO>(
        "Device Manager: Starting device [%u] %s...\n",
        dev, std::get<dev>(devices_names).data());

    if (dev_cont.alloc<dev>() < 0) {
        kserver->syslog.print<PANIC>(
            "Failed to allocate device [%u] %s. Exiting server...\n",
            dev, std::get<dev>(devices_names).data());

        kserver->exit_all = true;
        return;
    }

    std::get<dev - 2>(device_list)
        = std::make_unique<KDevice<dev>>(kserver, dev_cont.get<dev>());
    std::get<dev - 2>(is_started) = true;
}

template<device_id dev0, device_id... devs>
std::enable_if_t<0 == sizeof...(devs) && 2 <= dev0, void>
DeviceManager::start_impl(device_id dev)
{
    static_assert(dev0 < device_num, "");
    static_assert(dev0 >= 2, "");
    alloc_device<dev0>();
}

template<device_id dev0, device_id... devs>
std::enable_if_t<0 < sizeof...(devs) && 2 <= dev0, void>
DeviceManager::start_impl(device_id dev)
{
    static_assert(dev0 < device_num, "");
    static_assert(dev0 >= 2, "");

    dev == dev0 ? alloc_device<dev0>()
                : start_impl<devs...>(dev);
}

template<device_id... devs>
void DeviceManager::start(device_id dev, std::index_sequence<devs...>)
{
    start_impl<devs...>(dev);
}

int DeviceManager::init()
{
    if (ctx.init() < 0) {
        kserver->syslog.print<CRITICAL>(
                "Context initialization failed\n");
        return -1;
    }

    return 0;
}

template<device_id dev0, device_id... devs>
std::enable_if_t<0 == sizeof...(devs) && 2 <= dev0, int>
DeviceManager::execute_dev_impl(KDeviceAbstract *dev_abs, Command& cmd)
{
    static_assert(dev0 < device_num, "");
    static_assert(dev0 >= 2, "");
    return static_cast<KDevice<dev0>*>(dev_abs)->execute(cmd);
}

template<device_id dev0, device_id... devs>
std::enable_if_t<0 < sizeof...(devs) && 2 <= dev0, int>
DeviceManager::execute_dev_impl(KDeviceAbstract *dev_abs, Command& cmd)
{
    static_assert(dev0 < device_num, "");
    static_assert(dev0 >= 2, "");

    return dev_abs->kind == dev0 ? static_cast<KDevice<dev0>*>(dev_abs)->execute(cmd)
                                 : execute_dev_impl<devs...>(dev_abs, cmd);
}

template<device_id... devs>
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
        if (unlikely(! is_started[cmd.device - 2]))
            start(cmd.device, make_index_sequence_in_range<2, device_num>());

        return execute_dev(device_list[cmd.device - 2].get(), cmd,
                           make_index_sequence_in_range<2, device_num>());
    }
}

} // namespace kserver
