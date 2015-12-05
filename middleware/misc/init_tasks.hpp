/// @file init_tasks.hpp
///
/// @brief Task to be performed at init. Callable via the CLI.
/// @date 12/09/2015
///
/// (c) Koheron 2014-2015

#ifndef __MISC_INIT_HPP__
#define __MISC_INIT_HPP__

#include "../drivers/core/dev_mem.hpp"

//> \description Task to be performed at init. Callable via the CLI.
class InitTasks
{
  public:
    InitTasks(Klib::DevMem& dev_mem_);
    
    //> \description Display IP address last number onto the RedPitaya LEDs
    //> \io_type WRITE
    void show_ip_on_leds();
    
  private:
    Klib::DevMem& dev_mem;
};

#endif // __MISC_INIT_HPP__
