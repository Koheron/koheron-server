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
#define EXPAND_AS_START_DEVICE(num, name, operations ...)                          \
    device_list[num] = new name (static_cast<KServer*>(device_list[KSERVER]), dev_mem);
#else
#define EXPAND_AS_START_DEVICE(num, name, operations ...)                          \
    device_list[num] = new name (static_cast<KServer*>(device_list[KSERVER]));
#endif

DeviceManager::DeviceManager(KServer *kserver_)
: device_list(device_num)
,  kserver(kserver_)
#if KSERVER_HAS_DEVMEM
,  dev_mem()
#endif
{
    device_list[KSERVER] = static_cast<KDeviceAbstract*>(kserver);
    // Start all devices
    DEVICES_TABLE(EXPAND_AS_START_DEVICE)
}

// X Macro: Stop device
#define EXPAND_AS_STOP_DEVICE(num, name, operations ...)   \
    case num: {                                            \
        delete static_cast< name *>(device_list[num]);     \
        break;                                             \
    }

DeviceManager::~DeviceManager()
{
    // KServer is never destroyed
    for (unsigned int i=2; i<device_num; i++) {
        // A direct call to delete as:
        // delete device_list[dev];
        // doesn't call the specific destructor of each class
        // since device_list contains pointers of KDeviceAbstract.
        // Therefore, we cast to the appropriate KDevice before
        // deleting:
        // delete ( dev_name *) device_list[dev];
        switch ((device_t)i) {
          // Automatic generation with X macro
          DEVICES_TABLE(EXPAND_AS_STOP_DEVICE)

          default: assert(false);
        }
    }
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
        error = static_cast<KDevice<name, num> *>(dev_abs)->execute(cmd); \
        break;                                                            \
    }

int DeviceManager::Execute(Command& cmd)
{
    if (cmd.device == 0)
        return 0;

    assert(cmd.device < device_num);
    KDeviceAbstract *dev_abs = device_list[cmd.device];
    int error = 0;

    switch (dev_abs->kind) {
      case NO_DEVICE:
        return 0;
        break;
      case KSERVER: {
        error = static_cast<KDevice<KServer, KSERVER> *>(dev_abs)->execute(cmd);
        break;
      }

      DEVICES_TABLE(EXPAND_AS_EXECUTE_DEVICE) // X-Macro

      case device_num:
      default:
        kserver->syslog.print<SysLog::CRITICAL>("Execute: Unknown device\n");
        return -1;
    }

    return error; 
}

} // namespace kserver
