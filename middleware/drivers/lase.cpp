/// (c) Koheron

#include "lase.hpp"

Lase::Lase(Klib::DevMem& dev_mem_)
: dev_mem(dev_mem_),
  xadc(dev_mem_),
  gpio(dev_mem_)
{
    dac_wfm_size = 0;
    status = CLOSED;
}

Lase::~Lase()
{
    Close();
}

# define MAP_SIZE 4096

int Lase::Open(uint32_t dac_wfm_size_)
{  
    // Reopening
    if(status == OPENED && dac_wfm_size_ != dac_wfm_size) {
        Close();
    }

    if(status == CLOSED) {
        dac_wfm_size = dac_wfm_size_;
    
        // Initializes memory maps
        config_map = dev_mem.AddMemoryMap(CONFIG_ADDR, MAP_SIZE);
        
        if(static_cast<int>(config_map) < 0) {
            status = FAILED;
            return -1;
        }
        
        status_map = dev_mem.AddMemoryMap(STATUS_ADDR, MAP_SIZE);
        
        if(static_cast<int>(status_map) < 0) {
            status = FAILED;
            return -1;
        }
        
        dac_map = dev_mem.AddMemoryMap(DAC_ADDR, dac_wfm_size*MAP_SIZE/1024);
        
        if(static_cast<int>(dac_map) < 0) {
            status = FAILED;
            return -1;
        }
        
        // Open core drivers
        xadc.Open();
        gpio.Open();
        
        status = OPENED;
    }
    
    return 0;
}

void Lase::Close()
{
    if(status == OPENED) {
        dev_mem.RmMemoryMap(config_map);
        dev_mem.RmMemoryMap(status_map);
        dev_mem.RmMemoryMap(dac_map);
        
        xadc.Close();
        gpio.Close();
    
        status = CLOSED;
    }
}

