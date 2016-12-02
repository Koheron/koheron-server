/// Server devices
///
/// (c) Koheron

#ifndef __KDEVICE_HPP__
#define __KDEVICE_HPP__

#include <cstring>

#include "kserver_defs.hpp"
#include <devices_table.hpp>

namespace kserver {

class KServer;
struct Command;

class KDeviceAbstract {
  public:
    KDeviceAbstract(device_id kind_, KServer *kserver_)
    : kind(kind_)
    , kserver(kserver_)
    {}

    device_id kind = dev_id_of<NoDevice>;
    KServer *kserver;
friend class DeviceManager;
};

template<device_id dev_kind>
class KDevice : public KDeviceAbstract {};

} // namespace kserver

#endif // __KDEVICE_HPP__
