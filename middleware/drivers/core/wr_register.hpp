/// @file wr_register.hpp
///
/// @brief Memory map devices and write/read registers.
///
/// @author Thomas Vanderbruggen <thomas@koheron.com>
/// @date 02/08/2014
///
/// (c) Koheron 2014

#ifndef __DRIVERS_CORE_WR_REGISTER_HPP__
#define __DRIVERS_CORE_WR_REGISTER_HPP__

#include <bitset>
#include <cstdint>

/// @namespace Klib
/// @brief Namespace of the Koheron library
namespace Klib {

// -----------------------------------------------------------------------------
//			I/O access
// -----------------------------------------------------------------------------

/// @brief Write a value in a 32 bits register
/// @Addr Absolute address of the register to be written
/// @RegVal Value to be written (uint32)
inline void WriteReg32(intptr_t Addr, uint32_t RegVal)
{
    *(volatile intptr_t *) Addr = RegVal;
}

/// @brief Write a value in a 32 bits register
/// @Addr Absolute address of the register to be written
/// @RegVal Value to be written (bitset<32>)
inline void WriteReg32(intptr_t Addr, std::bitset<32> RegVal)
{
    *(volatile intptr_t *) Addr = RegVal.to_ulong();
}

/// @brief Write a buffer of 32 bits registers
/// @Addr Absolute address of the first register of the buffer
/// @data_ptr Pointer to the data to be written
/// @buff_size Number of data to write in the buffer
inline void WriteBuff32(intptr_t Addr, const uint32_t *data_ptr, 
                        uint32_t buff_size)
{
    for(uint32_t i=0; i < buff_size; i++) {
        WriteReg32(Addr + sizeof(uint32_t)*i, data_ptr[i]);
    }
}

/// @brief Read a value in a 32 bits register
/// @Addr Absolute address of the register to be read
inline uint32_t ReadReg32(intptr_t Addr)
{
    return *(volatile intptr_t *) Addr;
}

// -- Bit manipulations
//
// http://stackoverflow.com/questions/47981/how-do-you-set-clear-and-toggle-a-single-bit-in-c-c

/// @brief Set a bit in a 32 bits register
/// @Addr Absolute address of the register
/// @index Index of the bit in the register
inline void SetBit(intptr_t Addr, uint32_t Index)
{
    *(volatile intptr_t *) Addr = *((volatile intptr_t *) Addr) | (1 << Index);
}

/// @brief Clear a bit in a 32 bits register
/// @Addr Absolute address of the register
/// @index Index of the bit in the register
inline void ClearBit(intptr_t Addr, uint32_t Index)
{
    *(volatile intptr_t *) Addr = *((volatile intptr_t *) Addr) & ~(1 << Index);
}

/// @brief Toggle a bit in a 32 bits register
/// @Addr Absolute address of the register
/// @index Index of the bit in the register
inline void ToggleBit(intptr_t Addr, uint32_t Index)
{
    *(volatile intptr_t *) Addr = *((volatile intptr_t *) Addr) ^ (1 << Index);
}

/// @brief Obtain the value of a bit
/// @Addr Absolute address of the register
/// @index Index of the bit in the register
inline bool ReadBit(intptr_t Addr, uint32_t Index)
{
    return *((volatile intptr_t *) Addr) & (1 << Index);
}

}; // namespace Klib

#endif // __DRIVERS_CORE_WR_REGISTER_HPP__
