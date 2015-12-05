/// @file ks_dev_mem.hpp
///
/// (c) Koheron 2014-2015 

#ifndef __KS_DEV_MEM_HPP__
#define __KS_DEV_MEM_HPP__

#include <drivers/core/wr_register.hpp>
#include <drivers/core/dev_mem.hpp>

#if KSERVER_HAS_THREADS
#include <mutex>
#endif

#include "../core/kdevice.hpp"
#include "../core/devices_manager.hpp"

namespace kserver {

class KS_Dev_mem : public KDevice<KS_Dev_mem,DEV_MEM>
{
  public:
    const device_t kind = DEV_MEM;
    enum { __kind = DEV_MEM };

  public:
    KS_Dev_mem(KServer* kserver, Klib::DevMem& dev_mem_)
    : KDevice<KS_Dev_mem,DEV_MEM>(kserver)
    , dev_mem(dev_mem_)
    {}

    enum Operation {
        OPEN,
        ADD_MEMORY_MAP,
        RM_MEMORY_MAP,
        READ,
        WRITE,
        WRITE_BUFFER,
        READ_BUFFER,
        SET_BIT,
        CLEAR_BIT,
        TOGGLE_BIT,
        MASK_AND,
        MASK_OR,
        dev_mem_op_num
    };

#if KSERVER_HAS_THREADS
    std::mutex mutex;
#endif

    Klib::DevMem& dev_mem;
    
}; // class KS_Dev_mem

template<>
template<>
struct KDevice<KS_Dev_mem,DEV_MEM>::
            Argument<KS_Dev_mem::OPEN>
{};

template<>
template<>
struct KDevice<KS_Dev_mem,DEV_MEM>::
            Argument<KS_Dev_mem::ADD_MEMORY_MAP>
{
        unsigned int device_addr; ///< Physical address of the device
    
        unsigned int map_size; ///< Mmap size
    };

template<>
template<>
struct KDevice<KS_Dev_mem,DEV_MEM>::
            Argument<KS_Dev_mem::RM_MEMORY_MAP>
{Klib::MemMapID mmap_idx; ///< Index of Memory Map
    };

template<>
template<>
struct KDevice<KS_Dev_mem,DEV_MEM>::
            Argument<KS_Dev_mem::READ>
{Klib::MemMapID mmap_idx; ///< Index of Memory Map
    
        unsigned int offset; ///< Offset of the register to read
    };

template<>
template<>
struct KDevice<KS_Dev_mem,DEV_MEM>::
            Argument<KS_Dev_mem::WRITE>
{Klib::MemMapID mmap_idx; ///< Index of Memory Map
    
        unsigned int offset; ///< Offset of the register to read
    
        unsigned int reg_val; ///< Value to write in the register
    };

template<>
template<>
struct KDevice<KS_Dev_mem,DEV_MEM>::
            Argument<KS_Dev_mem::WRITE_BUFFER>
{Klib::MemMapID mmap_idx; ///< Index of Memory Map
    
        unsigned int offset; ///< Offset of the register to write
    
        unsigned int len_data; ///< Size of the buffer data. To be used for the handshaking.
    };

template<>
template<>
struct KDevice<KS_Dev_mem,DEV_MEM>::
            Argument<KS_Dev_mem::READ_BUFFER>
{Klib::MemMapID mmap_idx; ///< Index of Memory Map
    
        unsigned int offset; ///< Offset of the register to read
    
        unsigned int buff_size; ///< Number of registers to read
    };

template<>
template<>
struct KDevice<KS_Dev_mem,DEV_MEM>::
            Argument<KS_Dev_mem::SET_BIT>
{Klib::MemMapID mmap_idx; ///< Index of Memory Map
    
        unsigned int offset; ///< Offset of the register
    
        unsigned int index; ///< Index of the bit to set in the register
    };

template<>
template<>
struct KDevice<KS_Dev_mem,DEV_MEM>::
            Argument<KS_Dev_mem::CLEAR_BIT>
{Klib::MemMapID mmap_idx; ///< Index of Memory Map
    
        unsigned int offset; ///< Offset of the register
    
        unsigned int index; ///< Index of the bit to set in the register
    };

template<>
template<>
struct KDevice<KS_Dev_mem,DEV_MEM>::
            Argument<KS_Dev_mem::TOGGLE_BIT>
{Klib::MemMapID mmap_idx; ///< Index of Memory Map
    
        unsigned int offset; ///< Offset of the register
    
        unsigned int index; ///< Index of the bit to set in the register
    };

template<>
template<>
struct KDevice<KS_Dev_mem,DEV_MEM>::
            Argument<KS_Dev_mem::MASK_AND>
{Klib::MemMapID mmap_idx; ///< Index of Memory Map
    
        unsigned int offset; ///< Offset of the register
    
        unsigned int mask; ///< Mask to apply on the register
    };

template<>
template<>
struct KDevice<KS_Dev_mem,DEV_MEM>::
            Argument<KS_Dev_mem::MASK_OR>
{Klib::MemMapID mmap_idx; ///< Index of Memory Map
    
        unsigned int offset; ///< Offset of the register
    
        unsigned int mask; ///< Mask to apply on the register
    };

} // namespace kserver

#endif //__KS_DEV_MEM_HPP__
