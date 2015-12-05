/// @file ks_init_tasks.hpp
///
/// (c) Koheron 2014-2015 

#ifndef __KS_INIT_TASKS_HPP__
#define __KS_INIT_TASKS_HPP__

#include <misc/init_tasks.hpp>
#include <drivers/core/dev_mem.hpp>

#if KSERVER_HAS_THREADS
#include <mutex>
#endif

#include "../core/kdevice.hpp"
#include "../core/devices_manager.hpp"

namespace kserver {

class KS_Init_tasks : public KDevice<KS_Init_tasks,INIT_TASKS>
{
  public:
    const device_t kind = INIT_TASKS;
    enum { __kind = INIT_TASKS };

  public:
    KS_Init_tasks(KServer* kserver, Klib::DevMem& dev_mem_)
    : KDevice<KS_Init_tasks,INIT_TASKS>(kserver)
    , dev_mem(dev_mem_)
    , __init_tasks(dev_mem_)
    {}

    enum Operation {
        SHOW_IP_ON_LEDS,
        init_tasks_op_num
    };

#if KSERVER_HAS_THREADS
    std::mutex mutex;
#endif

    Klib::DevMem& dev_mem;
    InitTasks __init_tasks;
    
}; // class KS_Init_tasks

template<>
template<>
struct KDevice<KS_Init_tasks,INIT_TASKS>::
            Argument<KS_Init_tasks::SHOW_IP_ON_LEDS>
{};

} // namespace kserver

#endif //__KS_INIT_TASKS_HPP__
