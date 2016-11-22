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
#if KSERVER_HAS_DEVMEM
,  dev_mem()
#endif
{}

// X Macro: Start new device
#if KSERVER_HAS_DEVMEM
#define EXPAND_AS_START_DEVICE(num, name)            \
    device_list[num - 2] = std::make_unique<name>(kserver, dev_mem);
#else
#define EXPAND_AS_START_DEVICE(num, name)            \
    device_list[num - 2] = std::make_unique<name>(kserver);
#endif

int DeviceManager::init()
{
#if KSERVER_HAS_DEVMEM
    if (dev_mem.open() < 0) {
        kserver->syslog.print<SysLog::CRITICAL>(
                              "Can't start MemoryManager\n");
        return -1;
    }
#endif

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

template<device_t dev0, device_t... devs>
std::enable_if_t<dev0 == 0 || dev0 == 1, int> // Cases NO_DEVICE and KSERVER
execute_dev_impl(KDeviceAbstract *dev_abs, Command& cmd) {
    return execute_dev_impl<devs...>(dev_abs, cmd);
}

template<device_t... devs>
int execute_dev(KDeviceAbstract *dev_abs, Command& cmd,
                std::index_sequence<devs...>) {
    static_assert(sizeof...(devs) == device_num, "");
    return execute_dev_impl<devs...>(dev_abs, cmd);
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
                           std::make_index_sequence<device_num>());
}

} // namespace kserver
