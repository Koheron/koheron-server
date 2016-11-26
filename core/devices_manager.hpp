/// Server devices manager
///
/// (c) Koheron

#ifndef __DEVICES_MANAGER_HPP__
#define __DEVICES_MANAGER_HPP__

#include <array>
#include <memory>
#include <assert.h>

#include "kdevice.hpp"

#include <devices_container.hpp>

namespace kserver {

class KServer;
struct Command;

class DeviceManager
{
  public:
    DeviceManager(KServer *kserver_);

    int init();
    int execute(Command &cmd);

    template<device_t dev>
    auto& get() {
        if (! std::get<dev - 2>(is_started))
            alloc_device<dev>();
        return dev_cont.get<dev>();
    }

  private:
    // Store devices (except KServer) as unique_ptr
    std::array<std::unique_ptr<KDeviceAbstract>, device_num - 2> device_list;
    KServer *kserver;
    DevicesContainer dev_cont;
    std::array<bool, device_num - 2> is_started;

    template<std::size_t dev> void alloc_device();

    // Start

    template<device_t... devs>
    void start(device_t dev, std::index_sequence<devs...>);

    template<device_t dev0, device_t... devs>
    std::enable_if_t<0 == sizeof...(devs) && 2 <= dev0, void>
    start_impl(device_t dev);

    template<device_t dev0, device_t... devs>
    std::enable_if_t<0 < sizeof...(devs) && 2 <= dev0, void>
    start_impl(device_t dev);

    // Execute

    template<device_t... devs>
    int execute_dev(KDeviceAbstract *dev_abs, Command& cmd,
                    std::index_sequence<devs...>);

    template<device_t dev0, device_t... devs>
    std::enable_if_t<0 == sizeof...(devs) && 2 <= dev0, int>
    execute_dev_impl(KDeviceAbstract *dev_abs, Command& cmd);

    template<device_t dev0, device_t... devs>
    std::enable_if_t<0 < sizeof...(devs) && 2 <= dev0, int>
    execute_dev_impl(KDeviceAbstract *dev_abs, Command& cmd);
};

} // namespace kserver

#endif // __DEVICES_MANAGER_HPP__
