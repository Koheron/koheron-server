/// (c) Koheron

#ifndef __CONTEXT_HPP__
#define __CONTEXT_HPP__

#if KSERVER_HAS_DEVMEM
#include <drivers/lib/memory_manager.hpp>
#endif

struct Context {
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

#if KSERVER_HAS_DEVMEM
    MemoryManager mm;
#endif
};

#endif // __CONTEXT_HPP__