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

/// @brief Abstract class for KDevice
class KDeviceAbstract {
public:
    KDeviceAbstract(device_t kind_)
    :kind(kind_) {}

    device_t kind = NO_DEVICE;

    template<class Dev, device_t dev_kind>
    KDevice<Dev, dev_kind>* cast() {
        if (Dev::__kind != kind)
            return NULL;
        else
            return static_cast<KDevice<Dev, dev_kind>*>(this);
    }
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
    /// Contains the arguments of the operation op
    template <int op> struct Argument;

    KDevice(KServer *kserver_)
    : KDeviceAbstract(dev_kind),
      kserver(kserver_)
    {}

    int execute(Command& cmd);

  private:
    /// Each device knows the KServer class,
    /// which itself knows every body else
    KServer* kserver;

  protected:
    /// Parse the buffer of a command
    /// @cmd The Command to be parsed
    /// @args The arguments resulting of the parsing
    template<int op>
    int parse_arg(Command& cmd, Argument<op>& args, SessID sess_id);

    /// Execute an operation
    /// @args The arguments of the operation provided by @parse_arg
    /// @sess_id ID of the session executing the operation
    template<int op>
    int execute_op(const Argument<op>& args, SessID sess_id);

friend class DeviceManager;
friend Dev;
};

/// Macros to simplify edition of operations 
#define SEND kserver->session_manager.get_session(sess_id).send
#define SEND_CSTR kserver->session_manager.get_session(sess_id).send_cstr
#define RCV_VECTOR kserver->session_manager.get_session(sess_id).rcv_vector
#define DESERIALIZE kserver->session_manager.get_session(sess_id).deserialize
#define EXTRACT_ARRAY kserver->session_manager.get_session(sess_id).extract_array

} // namespace kserver

#endif // __KDEVICE_HPP__
