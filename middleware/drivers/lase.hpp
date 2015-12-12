/// Laser development kit driver
///
/// (c) Koheron

#ifndef __DRIVERS_LASE_HPP__
#define __DRIVERS_LASE_HPP__

#include "dev_mem.hpp"
#include "wr_register.hpp"

#define CONFIG_ADDR 0x60000000
#define STATUS_ADDR 0x50000000
#define DAC_ADDR    0x40000000

class Lase
{
  public:
    Lase(Klib::DevMem& dev_mem_);
    ~Lase();
  private:
    Klib::DevMem& dev_mem;
  
};

#endif // __DRIVERS_LASE_HPP__
