/// (c) Koheron

#include "dev_mem.hpp"

/// @namespace Klib
/// @brief Namespace of the Koheron library
namespace Klib {

MemMapID MemMapIdPool::get_id(unsigned int num_maps)
{
    MemMapID new_id;
    
    // Choose a reusable ID if available else
    // create a new ID equal to the number of memmaps
    if (reusable_ids.size() == 0) {    
        new_id = static_cast<MemMapID>(num_maps);
    } else {
        new_id = reusable_ids.back();
        reusable_ids.pop_back();
    }

    return new_id;
}

void MemMapIdPool::release_id(MemMapID id)
{
    reusable_ids.push_back(id);
}

DevMem::DevMem(intptr_t addr_limit_down_, intptr_t addr_limit_up_)
: addr_limit_down(addr_limit_down_),
  addr_limit_up(addr_limit_up_),
  mem_maps()
{
    fd = -1;
    is_open = 0;
}

DevMem::~DevMem()
{
    Close();
}

int DevMem::Open()
{
    // Open /dev/mem if not already open
    if (fd == -1) {
        fd = open("/dev/mem", O_RDWR | O_SYNC);

         if (fd == -1) {
            fprintf(stderr, "Can't open /dev/mem\n");
            return -1;
        }
        
        is_open = 1;
    }
	
    RemoveAll(); // Reset memory maps
    return fd;
}

int DevMem::Close()
{
    RemoveAll();
    close(fd);
    return 0;
}

unsigned int DevMem::num_maps = 0;

bool DevMem::__is_forbidden_address(intptr_t addr)
{
    if (addr_limit_up == 0x0 && addr_limit_down == 0x0)
        return false; // No limit defined
    else
        return (addr > addr_limit_up) || (addr < addr_limit_down);
}

template<size_t N>
std::array<MemMapID, N> 
DevMem::RequestMemoryMaps(std::array<MemoryRegion, N> regions)
{
    auto map_ids = std::array<MemMapID, N>();
    map_ids.fill(static_cast<MemMapID>(-1));
    uint32_t i = 0;

    for (auto& region : regions) {
        for (auto& mem_map : mem_maps) {
            // TODO Reallocate also if new range is larger
            if (region.phys_addr == mem_map.second->PhysAddr()) {
                map_ids[i] = mem_map.first;
                break;
            }
        }

        if (map_ids[i] < 0) // The required region is not mapped
            map_ids[i] = AddMemoryMap(region.phys_addr, region.range);

        i++;
    }

    return map_ids;
}

MemMapID DevMem::AddMemoryMap(intptr_t addr, uint32_t size)
{
    if (__is_forbidden_address(addr)) {
        fprintf(stderr,"Forbidden memory region\n");
        return static_cast<MemMapID>(-1);
    }

    auto mem_map = new MemoryMap(&fd, addr, size);
    assert(mem_map != nullptr);

    if (mem_map->GetStatus() != MemoryMap::MEMMAP_OPENED) {
        fprintf(stderr,"Can't open memory map\n");
        return static_cast<MemMapID>(-1);
    }
	
    MemMapID new_id = id_pool.get_id(num_maps);
    mem_maps.insert(std::pair<MemMapID, MemoryMap*>(new_id, mem_map));
    num_maps++;
    return new_id;
}

MemoryMap& DevMem::GetMemMap(MemMapID id)
{
    assert(mem_maps.at(id) != nullptr);
    return *mem_maps.at(id);
}

void DevMem::RmMemoryMap(MemMapID id)
{
    if (mem_maps[id] != nullptr)
        delete mem_maps[id];

    mem_maps.erase(id);
    id_pool.release_id(id);
    num_maps--;
}

void DevMem::RemoveAll()
{
    assert(num_maps == mem_maps.size());

    if (!mem_maps.empty()) {
        // This is necessary because both mem_maps.size() and num_maps
        // are decremented at each call of RmMemoryMap
        uint32_t mem_maps_size = mem_maps.size();
    
        for (unsigned int id=0; id<mem_maps_size; id++)
            RmMemoryMap(id);
    }
        
    assert(num_maps == 0);
}

uint32_t DevMem::GetBaseAddr(MemMapID id)
{
    assert(mem_maps.at(id) != nullptr);
    return mem_maps.at(id)->GetBaseAddr();
}

int DevMem::GetStatus(MemMapID id)
{
    assert(mem_maps.at(id) != nullptr);   
    return mem_maps.at(id)->GetStatus();
}

int DevMem::IsFailed()
{
    for (unsigned int i=0; i<mem_maps.size(); i++)
        if (mem_maps[i]->GetStatus() == MemoryMap::MEMMAP_FAILURE)
            return 1;
    return 0;
}

}; // namespace Klib

