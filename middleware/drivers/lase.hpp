/// Laser development kit driver
///
/// (c) Koheron

#ifndef __DRIVERS_LASE_HPP__
#define __DRIVERS_LASE_HPP__

#include "core/dev_mem.hpp"
#include "core/wr_register.hpp"
#include "core/xadc.hpp"
#include "core/gpio.hpp"

// Addresses
#define CONFIG_ADDR      0x60000000
#define STATUS_ADDR      0x50000000
#define DAC_ADDR         0x40000000

// Config offsets
#define LEDS_OFF         0
#define PWM0_OFF         4
#define PWM1_OFF         8
#define PWM2_OFF         12
#define PWM3_OFF         16
#define ADDR_OFF         20
#define AVG1_OFF         24
#define AVG2_OFF         28
#define BITSTREAM_ID_OFF 36

// Status offsets
#define N_AVG1_OFF       0
#define N_AVG2_OFF       0 // 4 ??

class Lase
{
  public:
    Lase(Klib::DevMem& dev_mem_);
    ~Lase();
    
    //> \description Open the device
    //> \io_type WRITE
    //> \status ERROR_IF_NEG
    //> \on_error Cannot open LASE device
    //> \flag AT_INIT
    int Open(uint32_t dac_wfm_size_);
    
    void Close();
    
    enum Status {
        CLOSED,
        OPENED,
        FAILED
    };
    
    //> \is_failed
    bool IsFailed() const {return status == FAILED;}
    
  private:
    // Core drivers
    Klib::DevMem& dev_mem;
    Xadc xadc;
    Gpio gpio;
        
    int status;
    
    // Number of point in the DAC waveform
    uint32_t dac_wfm_size;
    
    // Memory maps IDs:
    Klib::MemMapID config_map;
    Klib::MemMapID status_map;
    Klib::MemMapID dac_map;
};

#endif // __DRIVERS_LASE_HPP__
