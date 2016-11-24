/// Server devices
///
/// (c) Koheron

#ifndef __KDEVICE_HPP__
#define __KDEVICE_HPP__

#include <cstring>

#include "kserver_defs.hpp"
#include "dev_definitions.hpp"

namespace kserver {

class KServer;
struct Command;

class KDeviceAbstract {
  public:
    KDeviceAbstract(device_t kind_, KServer *kserver_)
    : kind(kind_)
    , kserver(kserver_)
    {}

    device_t kind = NO_DEVICE;
    KServer *kserver;
friend class DeviceManager;
};

template<device_t dev_kind>
class KDevice : public KDeviceAbstract {
  public:
    KDevice(KServer *kserver_);

    int execute(Command& cmd);

    // XXX Can we do something less ugly that void* for type erasure
    const void* get_device_ptr() const;
};

} // namespace kserver

#endif // __KDEVICE_HPP__
