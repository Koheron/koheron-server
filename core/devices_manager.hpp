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
        return dev_cont.get<dev>();
    }

  private:
    // Store devices (except KServer) as unique_ptr
    std::array<std::unique_ptr<KDeviceAbstract>, device_num - 2> device_list;
    KServer *kserver;
    DevicesContainer dev_cont;

    template<std::size_t dev> void alloc_device();

    template<std::size_t num>
    std::enable_if_t<num < 2, void>
    open_devices() {}

    template<std::size_t num>
    std::enable_if_t<num >= 2, void>
    open_devices() {
        alloc_device<num>();
        open_devices<num - 1>();
    }
};

} // namespace kserver

#endif // __DEVICES_MANAGER_HPP__
