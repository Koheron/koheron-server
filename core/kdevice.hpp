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

class KDeviceAbstract {
public:
    KDeviceAbstract(device_t kind_)
    : kind(kind_) {}

    device_t kind = NO_DEVICE;
};

struct Command;

template<device_t dev_kind>
class KDevice : public KDeviceAbstract
{
  public:
    KDevice(KServer *kserver_)
    : KDeviceAbstract(dev_kind),
      kserver(kserver_)
    {}

    int execute(Command& cmd);

  private:
    KServer *kserver;

  protected:
    template<int op> int execute_op(Command& cmd);

friend class DeviceManager;
};

} // namespace kserver

#endif // __KDEVICE_HPP__
