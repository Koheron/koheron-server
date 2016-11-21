/// Server devices
///
/// (c) Koheron

#ifndef __KDEVICE_HPP__
#define __KDEVICE_HPP__

#include <cstring> 

#include "kserver_defs.hpp"
#include "dev_definitions.hpp"

namespace kserver {

// ---------------------------------------------
// KDevice
// ---------------------------------------------

template<class Dev, device_t dev_kind> class KDevice;

// X Macro: Device class list
#define EXPAND_AS_CLASS_LIST(num, name, operations ...)     \
        class name;

class KServer;
DEVICES_TABLE(EXPAND_AS_CLASS_LIST) // X-macro

class KDeviceAbstract {
public:
    KDeviceAbstract(device_t kind_)
    :kind(kind_) {}

    device_t kind = NO_DEVICE;
};

struct Command;

/// Polymorph class for a KServer device
/// 
/// Uses static polymorphism with Curiously Recurring Template Pattern (CRTP)
/// http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern 
template<class Dev, device_t dev_kind>
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
friend Dev;
};

/// Macros to simplify edition of operations
#define RECV kserver->session_manager.get_session(cmd.sess_id).recv
#define DESERIALIZE kserver->session_manager.get_session(cmd.sess_id).deserialize
#define SEND kserver->session_manager.get_session(cmd.sess_id).send

} // namespace kserver

#endif // __KDEVICE_HPP__
