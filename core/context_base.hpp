/// (c) Koheron

#ifndef __CONTEXT_BASE_HPP__
#define __CONTEXT_BASE_HPP__

#include <core/kserver_defs.hpp>
#include <devices_table.hpp>

namespace kserver {
    class DeviceManager;
    class KServer;
}

class ContextBase
{
  private:
    kserver::DeviceManager *dm;

    void set_device_manager(kserver::DeviceManager *dm_) {
        dm = dm_;
    }

  public:
    template<class Dev>
    Dev& get() const;

  protected:
    virtual int init() { return 0; }

friend class kserver::DeviceManager;
friend class kserver::KServer;
};

#endif // __CONTEXT_BASE_HPP__