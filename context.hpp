/// (c) Koheron

#ifndef __CONTEXT_HPP__
#define __CONTEXT_HPP__

#include <core/context_base.hpp>

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