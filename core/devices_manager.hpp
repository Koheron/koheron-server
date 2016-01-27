/// Server devices manager
///
/// (c) Koheron

#ifndef __DEVICES_MANAGER_HPP__
#define __DEVICES_MANAGER_HPP__

#include <array>
#include <bitset>
#include <assert.h>

#include "kdevice.hpp"

// XXX This must be at the end else compile error:
// error: ‘mutex’ in namespace ‘std’ does not name a type
// with gcc (Ubuntu 4.8.4-2ubuntu1~14.04) 4.8.4
#if KSERVER_HAS_THREADS
#  include <mutex>
#endif

#include <drivers/dev_mem.hpp>

namespace kserver {

/// Status of a device
typedef enum {
    DEV_OFF,	///< Device OFF
    DEV_ON,		///< Device ON
    DEV_FAIL,	///< Device failed to start
    KS_device_status_num
} KS_device_status;

/// Device status descriptions
const std::array< std::string, KS_device_status_num > 
KS_dev_status_desc = {{
    "OFF",
    "ON",
    "FAIL"
}};

class KServer;
struct Command;

class DeviceManager
{
  public:
    DeviceManager(KServer *kserver_);

    ~DeviceManager();
    
    int Init();

    /// Start device
    int StartDev(device_t dev);

    /// Start all devices
    /// @return 0 if success, -1 if failure
    int StartAll(void);

    /// Stop device
    void StopDev(device_t dev);

    /// Reset all devices
    void Reset(void);

    /// Execute a command
    int Execute(const Command &cmd);

    /// Return true if the device is already started
    bool IsStarted(device_t dev) const;

    /// Set a Device to started
    void SetDevStarted(device_t dev);

    /// Return true if a device failed to open/initialize
    bool IsFailed(device_t dev);

    /// Device status
    KS_device_status GetStatus(device_t dev);
    
    Klib::DevMem& GetDevMem() {return dev_mem;}

  private:
    std::vector<KDeviceAbstract*> device_list;
    KServer *kserver;

    Klib::DevMem dev_mem;

    /// True if a device is started
    std::bitset<device_num> is_started;
    
#if KSERVER_HAS_THREADS
    std::mutex mutex;
#endif
};

} // namespace kserver

#include "../devices/devices.hpp"

#endif // __DEVICES_MANAGER_HPP__

