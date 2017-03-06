/// (c) Koheron

#ifndef __CONTEXT_HPP__
#define __CONTEXT_HPP__

#include <core/kserver_defs.hpp>
#include <core/syslog.hpp>
#include <devices_table.hpp>

#if KSERVER_HAS_DEVMEM
#include <drivers/lib/memory_manager.hpp>
#include <drivers/memory.hpp>
#endif

namespace kserver {
    class DeviceManager;
    class KServer;
}

class Context {
  private:
    kserver::DeviceManager& dm;
    kserver::SysLog& syslog;

  public:
#if KSERVER_HAS_DEVMEM
    MemoryManager mm;
#endif

    template<class Dev>
    Dev& get() const;

    template<unsigned int severity, typename... Args>
    void log(const char *msg, Args&&... args) {
        syslog.print<severity>(msg, std::forward<Args>(args)...);
    }

  private:
    Context(kserver::DeviceManager& dm_,
            kserver::SysLog& syslog_)
    : dm(dm_)
    , syslog(syslog_)
#if KSERVER_HAS_DEVMEM
    , mm()
#endif
    {}

    int init() {
#if KSERVER_HAS_DEVMEM
        if (mm.open() < 0)
            return -1;
#endif
        return 0;
    }

friend class kserver::DeviceManager;
friend class kserver::KServer;
};

#endif // __CONTEXT_HPP__