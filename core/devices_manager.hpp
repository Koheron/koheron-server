/// Server devices manager
///
/// (c) Koheron

#ifndef __DEVICES_MANAGER_HPP__
#define __DEVICES_MANAGER_HPP__

#include <array>
#include <memory>
#include <assert.h>

#include "kdevice.hpp"

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
    const auto& get() const {
        return static_cast<KDevice<dev>*>(std::get<dev - 2>(device_list).get())->get_device();
    }

  private:
    // Store devices (except KServer) as unique_ptr
    std::array<std::unique_ptr<KDeviceAbstract>, device_num - 2> device_list;
    KServer *kserver;
};

} // namespace kserver

#endif // __DEVICES_MANAGER_HPP__
