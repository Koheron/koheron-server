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
};

// template<device_t dev_kind> class KDevice ;

template<device_t dev_kind>
class KDeviceBase : public KDeviceAbstract
{
  public:
    KDeviceBase(KServer *kserver_)
    : KDeviceAbstract(dev_kind, kserver)
    {}

    int execute(Command& cmd);

  protected:
    template<int op> int execute_op(Command& cmd);

friend class DeviceManager;
};

template<device_t dev_kind>
class KDevice : public KDeviceBase<dev_kind> {};

} // namespace kserver

#endif // __KDEVICE_HPP__
