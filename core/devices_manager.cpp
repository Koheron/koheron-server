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

// X Macro: Start new device
#if KSERVER_HAS_DEVMEM
#define EXPAND_AS_START_DEVICE(num, name, operations ...)            \
    device_list[num - 2] = std::make_unique<name>(kserver, dev_mem);
#else
#define EXPAND_AS_START_DEVICE(num, name, operations ...)            \
    device_list[num - 2] = std::make_unique<name>(kserver);
#endif

DeviceManager::DeviceManager(KServer *kserver_)
: kserver(kserver_)
#if KSERVER_HAS_DEVMEM
,  dev_mem()
#endif
{
    // Start all devices
    DEVICES_TABLE(EXPAND_AS_START_DEVICE)
}

int DeviceManager::Init()
{
#if KSERVER_HAS_DEVMEM
    if (dev_mem.open() < 0) {
        kserver->syslog.print<SysLog::CRITICAL>(
                              "Can't start MemoryManager\n");
        return -1;
    }
#endif

    return 0;
}

// X Macro: Execute device
#define EXPAND_AS_EXECUTE_DEVICE(num, name, operations ...)               \
    case num: {                                                           \
        error = static_cast<KDevice<name, num>*>(dev_abs)->execute(cmd);  \
        break;                                                            \
    }

int DeviceManager::Execute(Command& cmd)
{
    assert(cmd.device < device_num);
    int error = 0;

    if (cmd.device == 0) return 0;
    if (cmd.device == 1) return kserver->execute(cmd);

    KDeviceAbstract *dev_abs = device_list[cmd.device - 2].get();

    switch (dev_abs->kind) {
      DEVICES_TABLE(EXPAND_AS_EXECUTE_DEVICE) // X-Macro

      case device_num:
      default: assert(false);
    }

    return error; 
}

} // namespace kserver
