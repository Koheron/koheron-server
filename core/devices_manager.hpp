/// Server devices manager
///
/// (c) Koheron

#ifndef __DEVICES_MANAGER_HPP__
#define __DEVICES_MANAGER_HPP__

#include <array>
#include <memory>
#include <assert.h>

#include "kdevice.hpp"

#if KSERVER_HAS_DEVMEM
#include <drivers/lib/memory_manager.hpp>
#endif

namespace kserver {

class KServer;
struct Command;

class DeviceManager
{
  public:
    DeviceManager(KServer *kserver_);
    
    int init();
    int execute(Command &cmd);

  private:
    // Store devices (except KServer) as unique_ptr
    std::array<std::unique_ptr<KDeviceAbstract>, device_num - 2> device_list;
    KServer *kserver;

#if KSERVER_HAS_DEVMEM
    MemoryManager dev_mem;
#endif
};

} // namespace kserver

#include <devices.hpp>

#endif // __DEVICES_MANAGER_HPP__

