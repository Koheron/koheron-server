/// Implementation of devices_manager.hpp
///
/// (c) Koheron

#include "devices_manager.hpp"
#include "kserver.hpp"
#include "commands.hpp"

#if KSERVER_HAS_THREADS
#  include <thread>
#endif

namespace kserver {

DeviceManager::DeviceManager(KServer *kserver_)
: device_list(device_num)
,  kserver(kserver_)
#if KSERVER_HAS_DEVMEM
,  dev_mem(kserver_->config->addr_limit_down, kserver_->config->addr_limit_up)
#endif
{
    device_list[KSERVER] = static_cast<KDeviceAbstract*>(kserver);
    is_started[KSERVER] = 1;
}

DeviceManager::~DeviceManager()
{
    Reset();
}

int DeviceManager::Init()
{
#if KSERVER_HAS_DEVMEM
    if (dev_mem.Open() < 0) {
        kserver->syslog.print(SysLog::CRITICAL,
                              "Can't start DevMem\n");
        return -1;
    }
#endif

    return 0;
}

// X Macro: Start new device
#if KSERVER_HAS_DEVMEM
#define EXPAND_AS_START_DEVICE(num, name, operations ...)                 \
        case num:                                                         \
            device_list[num]                                              \
                = new name (static_cast<KServer*>(device_list[KSERVER]),  \
                            dev_mem);                                     \
            break;
#else
#define EXPAND_AS_START_DEVICE(num, name, operations ...)                 \
        case num:                                                         \
            device_list[num]                                              \
                = new name (static_cast<KServer*>(device_list[KSERVER])); \
            break;
#endif

int DeviceManager::StartDev(device_t dev)
{
#if KSERVER_HAS_THREADS
    std::lock_guard<std::mutex> lock(mutex);
#endif

    assert(dev < device_num);

    if (is_started[dev])
        return 0;

    if (dev == NO_DEVICE) {
        is_started[dev] = 1;
        return 0;
    }

    if (dev == KSERVER)
        if (!is_started[dev]) {
            kserver->syslog.print(SysLog::CRITICAL,
                                  "KServer must always be started !\n");
            return -1;   
        }

    switch (dev) {
        // Automatic generation with X macro
        DEVICES_TABLE(EXPAND_AS_START_DEVICE)

      default:
        kserver->syslog.print(SysLog::CRITICAL, "Unknown device\n");
        return -1;
    }

    assert(device_list.at(dev) != NULL);

    if (IsFailed(dev)) {
        kserver->syslog.print(SysLog::CRITICAL, "Failed to start %s\n", 
                              GET_DEVICE_NAME(dev).c_str() );
        return -1;
    }

    is_started[dev] = 1;

    return 0;
}

// X Macro: Execute device
#define EXPAND_AS_EXECUTE_DEVICE(num, name, operations ...)         \
        case num: {                                                 \
            KDevice<name, num>                                      \
                *dev = static_cast<KDevice<name, num> *>(dev_abs);  \
            error = dev->execute(cmd);                              \
            break;                                                  \
        }

int DeviceManager::Execute(const Command& cmd)
{
    if (!is_started[cmd.device])
        if (StartDev(cmd.device) < 0)
            return -1;

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
        KDevice<KServer, KSERVER>
           *dev = static_cast<KDevice<KServer, KSERVER> *>(dev_abs);
        error = dev->execute(cmd);
        break;
      }

      DEVICES_TABLE(EXPAND_AS_EXECUTE_DEVICE) // X-Macro

      case device_num:
      default:
        kserver->syslog.print(SysLog::CRITICAL, "Execute: Unknown device\n");
        return -1;
    }

    return error; 
}

bool DeviceManager::IsStarted(device_t dev) const
{
    assert(dev < device_num);
    return is_started[(unsigned int) (dev)];
}

void DeviceManager::SetDevStarted(device_t dev)
{
    assert(dev < device_num);
    is_started[(unsigned int) (dev)] = 1;
}

bool DeviceManager::IsFailed(device_t dev)
{
    // NO_DEVICE never fail
    if (dev == NO_DEVICE)
        return 0;

    assert(dev < device_num);
    assert(device_list.at(dev) != 0);

    return device_list.at(dev)->is_failed();
}

// X Macro: Stop device
#define EXPAND_AS_STOP_DEVICE(num, name, operations ...)       \
        case num: {                                            \
            if (is_started[num]) {                             \
                delete static_cast< name *>(device_list[num]); \
                is_started[num] = 0;                           \
            }                                                  \
            break;                                             \
        }

void DeviceManager::StopDev(device_t dev)
{
#if KSERVER_HAS_THREADS
    std::lock_guard<std::mutex> lock(mutex);
#endif

    assert(dev < device_num);

    // A direct call to delete as:
    // delete device_list[dev];
    // doesn't call the specific destructor of each class
    // since device_list contains pointers of KDeviceAbstract.
    // Therefore, we cast to the appropriate KDevice before
    // deleting:
    // delete ( dev_name *) device_list[dev];
    switch (dev) {
      // Automatic generation with X macro
      DEVICES_TABLE(EXPAND_AS_STOP_DEVICE)

      default:
        kserver->syslog.print(SysLog::CRITICAL, "Unknown device\n");
    }
}

void DeviceManager::Reset(void) 
{
#if KSERVER_HAS_THREADS
    std::lock_guard<std::mutex> lock(mutex);
#endif

    // KServer is never reseted
    for (unsigned int i=2; i<device_num; i++)
        StopDev((device_t)i);
}

int DeviceManager::StartAll(void)
{
#if KSERVER_HAS_THREADS
    std::lock_guard<std::mutex> lock(mutex);
#endif

    int ret = 0;

    // Maybe not the most efficient implementation
    // But not a speed critical function
    for (unsigned int i=0; i<device_num; i++)
        if (StartDev((device_t)i) < 0)
            ret = -1;

    return ret;
}

KS_device_status DeviceManager::GetStatus(device_t dev)
{
    // NO_DEVICE and KSERVER are always on
    // and never fail
    if (dev==0 || dev==1)
        return DEV_ON;

    assert(dev < device_num);

    if (!is_started[dev])
        return DEV_OFF;

    if (IsFailed(dev))
        return DEV_FAIL;
    else
        return DEV_ON;
}

} // namespace kserver
