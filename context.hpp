/// (c) Koheron

#ifndef __CONTEXT_HPP__
#define __CONTEXT_HPP__

#include <core/kserver_defs.hpp>
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

  public:
#if KSERVER_HAS_DEVMEM
    MemoryManager mm;
#endif

    template<class Dev>
    Dev& get() const;

  private:
    Context(kserver::DeviceManager& dm_)
    : dm(dm_)
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