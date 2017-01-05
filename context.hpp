/// (c) Koheron

#ifndef __CONTEXT_HPP__
#define __CONTEXT_HPP__

#include <core/kserver_defs.hpp>
#include <core/syslog.hpp>
#include <devices_table.hpp>

namespace kserver {
    class DeviceManager;
    class KServer;
}

class ContextBase {
  private:
    kserver::DeviceManager& dm;
    kserver::SysLog& syslog;

  public:
    template<class Dev>
    Dev& get() const;

    template<unsigned int severity, typename... Args>
    void log(const char *msg, Args&&... args) {
        syslog.print<severity>(msg, std::forward<Args>(args)...);
    }

    template<class Dev, typename... Args>
    int notify(Args&&... args) {
        return syslog.notify<kserver::PubSub::DEVICES_CHANNEL,
                             dev_id_of<Dev>>(std::forward<Args>(args)...);
    }

  protected:
    ContextBase(kserver::DeviceManager& dm_,
                kserver::SysLog& syslog_)
    : dm(dm_)
    , syslog(syslog_)
    {}

    virtual int init() { return 0; }

friend class kserver::DeviceManager;
friend class kserver::KServer;
};

#if KSERVER_HAS_DEVMEM // TODO Move into koheron-sdk

#include <drivers/lib/memory_manager.hpp>
#include <drivers/memory.hpp>

class Context : public ContextBase
{
  public:
    Context(kserver::DeviceManager& dm,
            kserver::SysLog& log)
    : ContextBase(dm, log)
    , mm()
    {}

    int init() {
        return mm.open();
    }

    MemoryManager mm;
};

#else

class Context : public ContextBase
{
  public:
    Context(kserver::DeviceManager& dm,
            kserver::SysLog& log)
    : ContextBase(dm, log) {}
};

#endif

#endif // __CONTEXT_HPP__