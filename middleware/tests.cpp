/// (c) Koheron 

#include "tests.hpp"

#include <cmath>
#include <random>
#include <thread>

Tests::Tests(Klib::DevMem& dvm_unused_)
: data(0)
, buffer(0)
{
    waveform_size = 0;
    mean = 0;
    std_dev = 0;
}

int Tests::Open(uint32_t waveform_size_)
{ 
    data = std::vector<float>(waveform_size);
    return 0;
}

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

float* Tests::get_array_bis()
{
    std::default_random_engine generator(std::random_device{}());
    std::normal_distribution<float> distribution(mean, std_dev);

    for (unsigned int i=0; i<data.size(); i++)
       data[i] = distribution(generator);

    return data.data();
}

void Tests::set_buffer(const uint32_t *data, uint32_t len)
{
    buffer.resize(len);

    for (unsigned int i=0; i<buffer.size(); i++) {
        buffer[i] = data[i];
        printf("%u => %u\n", i, buffer[i]);
    }
}

const char* Tests::get_cstr()
{
    return "Hello !";
}

// -----------------------------------------------
// Send integers
// -----------------------------------------------

uint64_t Tests::read64()
{
    return (1ULL << 63);
}


int Tests::read_int()
{
    printf("Send -42\n");
    return -42;
}

unsigned int Tests::read_uint()
{
    return 42;
}

unsigned long Tests::read_ulong()
{
    return 2048;
}

unsigned long long Tests::read_ulonglong()
{
    return (1ULL << 63);
}

float Tests::read_float()
{
    return 0.42;
}

bool Tests::read_bool()
{
    return true;   
}