/// (c) Koheron 

#include "tests.hpp"

#include <cmath>
#include <random>
#include <thread>

Tests::Tests(Klib::DevMem& dvm_unused_)
: data(0)
, buffer(0)
{}

bool Tests::set_float(float f)
{
    return f == 12.5;
}

std::vector<float>& Tests::send_std_vector()
{
    data.resize(10);

    for (unsigned int i=0; i<data.size(); i++)
        data[i] = i*i*i;

    return data;
}

std::array<float, 10>& Tests::send_std_array()
{    
    for (uint32_t i=0; i<data_std_array.size(); i++)
        data_std_array[i] = i*i;

    return data_std_array;
}

float* Tests::send_c_array1(uint32_t n_pts)
{
    if (data.size() < 2*n_pts)
        data.resize(2*n_pts);

    for (unsigned int i=0; i<2*n_pts; i++)
        data[i] = static_cast<float>(i)/2;

    return data.data();
}

float* Tests::send_c_array2()
{
    data.resize(10);

    for (unsigned int i=0; i<data.size(); i++)
       data[i] = static_cast<float>(i)/4;

    return data.data();
}

bool Tests::set_buffer(const uint32_t *data, uint32_t len)
{
    if (len != 10)
        return false;

    bool is_ok = true;

    for (unsigned int i=0; i<len; i++) {
        if (data[i] != i*i) {
            is_ok = false;
            break;
        }
    }

    return is_ok;
}

const char* Tests::get_cstr()
{
    return "Hello !";
}

std::tuple<int, float, double> Tests::get_tuple()
{
    return std::make_tuple(2, 3.14159F, 2345.6);
}

uint64_t           Tests::read64()         { return (1ULL << 63); }
int                Tests::read_int()       { return -42;          }
unsigned int       Tests::read_uint()      { return 42;           }
unsigned long      Tests::read_ulong()     { return 2048;         }
unsigned long long Tests::read_ulonglong() { return (1ULL << 63); }
float              Tests::read_float()     { return 0.42;         }
bool               Tests::read_bool()      { return true;         }