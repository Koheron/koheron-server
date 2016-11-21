/// Server devices manager
///
/// (c) Koheron

#ifndef __DEVICES_MANAGER_HPP__
#define __DEVICES_MANAGER_HPP__

#include <array>
#include <bitset>
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

    ~DeviceManager();
    
    int Init();
    int Execute(Command &cmd);

  private:
    std::vector<KDeviceAbstract*> device_list;
    KServer *kserver;

#if KSERVER_HAS_DEVMEM
    MemoryManager dev_mem;
#endif
};

} // namespace kserver

#include <devices.hpp>

#endif // __DEVICES_MANAGER_HPP__

