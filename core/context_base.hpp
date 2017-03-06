/// (c) Koheron

#ifndef __CONTEXT_BASE_HPP__
#define __CONTEXT_BASE_HPP__

#include <core/kserver_defs.hpp>
#include <core/syslog.hpp>
#include <devices_table.hpp>

namespace kserver {
    class DeviceManager;
    class KServer;
}

class ContextBase
{
  private:
    kserver::DeviceManager *dm;
    kserver::SysLog *syslog;

    void set_device_manager(kserver::DeviceManager *dm_) {
        dm = dm_;
    }

    void set_syslog(kserver::SysLog *syslog_) {
        syslog = syslog_;
    }

  public:
    template<class Dev>
    Dev& get() const;

    template<unsigned int severity, typename... Args>
    void log(const char *msg, Args&&... args) {
        syslog->print<severity>(msg, std::forward<Args>(args)...);
    }

  protected:
    virtual int init() { return 0; }

friend class kserver::DeviceManager;
friend class kserver::KServer;
};

#endif // __CONTEXT_BASE_HPP__