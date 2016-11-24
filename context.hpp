/// (c) Koheron

#ifndef __CONTEXT_HPP__
#define __CONTEXT_HPP__

#include <core/kserver_defs.hpp>

#if KSERVER_HAS_DEVMEM
#include <drivers/lib/memory_manager.hpp>
#include <drivers/memory.hpp>
#endif

namespace kserver {
    class DeviceManager;
    class KServer;
}

struct Context {
#if KSERVER_HAS_DEVMEM
    MemoryManager mm;
#endif

    // TODO Make it const
    template<class Dev>
    Dev& get();

  private:
    kserver::DeviceManager& dm;

    // Constructor and initilization are private:
    // Devices cannot call them.

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