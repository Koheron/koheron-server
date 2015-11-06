/// @file dev_mem.hpp
///
/// @brief Device memory manager
///
/// @author Thomas Vanderbruggen <thomas@koheron.com>
/// @date 02/08/2014
///
/// (c) Koheron 2014-2015

#ifndef __DRIVERS_CORE_DEV_MEM_HPP__
#define __DRIVERS_CORE_DEV_MEM_HPP__

#include <map>
#include <vector>
#include <assert.h> 

extern "C" {
    #include <fcntl.h>
}

#include "memory_map.hpp"

/// @namespace Klib
/// @brief Namespace of the Koheron library
namespace Klib {

/// ID of a memory map
typedef uint32_t MemMapID;

/// @brief Device memory manager
/// Based on a memory maps factory
class DevMem
{
  public:
    DevMem();
    ~DevMem();

    /// @brief Open the /dev/mem driver
    int Open();
	
    /// @brief Close all the memory maps
    /// @return 0 if succeed, -1 else
    int Close();

    /// Current number of memory maps
    static unsigned int num_maps;

    /// @brief Create a new memory map
    /// @addr Base address of the map
    /// @size Size of the map 
    /// @return An ID to the created map,
    ///         or -1 if an error occured
    MemMapID AddMemoryMap(uint32_t addr, uint32_t size);
    
    /// @brief Remove a memory map
    /// @id ID of the memory map to be removed
    void RmMemoryMap(MemMapID id);
    
    /// @brief Remove all the memory maps
    void RemoveAll();
    
    /// @brief Get a memory map
    /// @id ID of the memory map
    MemoryMap& GetMemMap(MemMapID id);
    
    /// @brief Return the base address of a map
    /// @id ID of the map
    uint32_t GetBaseAddr(MemMapID id);
    
    /// @brief Return the status of a map
    /// @id ID of the map
    int GetStatus(MemMapID id);
	
    /// @brief Return 1 if a memory map failed
    int IsFailed();
    
    /// @brief True if the /dev/mem device is open
    inline bool IsOpen() const {return is_open;}

  private:
    int fd;         ///< /dev/mem file ID
    bool is_open;   ///< True if /dev/mem open
    
    /// Memory maps container
    std::map<MemMapID, MemoryMap*> mem_maps; 
    std::vector<MemMapID> reusable_ids;
};

}; // namespace Klib

#endif // __DRIVERS_CORE_DEV_MEM_HPP__
