/// (c) Koheron

#ifndef __CONTEXT_HPP__
#define __CONTEXT_HPP__

#include <core/kserver_defs.hpp>

#if KSERVER_HAS_DEVMEM
#include <drivers/lib/memory_manager.hpp>
#include <drivers/memory.hpp>
#endif

namespace kserver { class DeviceManager; }

struct Context {

#if KSERVER_HAS_DEVMEM
    MemoryManager mm;
#endif

  private:

    // Constructor and initilization are private:
    // Only the DeviceManager can call them.

    Context()
#if KSERVER_HAS_DEVMEM
    : mm()
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
};

#endif // __CONTEXT_HPP__