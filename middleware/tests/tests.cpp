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
    status = CLOSED;
}
 
Tests::~Tests()
{
    Close();
}

int Tests::Open(uint32_t waveform_size_)
{
    // Reopening
    if (status == OPENED && waveform_size_ != waveform_size)
        Close();

    if (status == CLOSED) {
        waveform_size = waveform_size_;        
        data = std::vector<float>(waveform_size);       
        status = OPENED;
    }
    
    return 0;
}

void Tests::Close()
{
    if (status == OPENED)
        status = CLOSED;
}

std::vector<float>& Tests::read()
{
    std::default_random_engine generator(std::random_device{}());
    std::normal_distribution<float> distribution(mean, std_dev);

    for (unsigned int i=0; i<data.size(); i++)
        data[i] = distribution(generator);

    return data;
}

void Tests::set_mean(float mean_)
{
    mean = mean_;
}

void Tests::set_std_dev(float std_dev_)
{
    std_dev = std_dev_;
}

std::array<float, 10>& Tests::send_std_array()
{    
    std::default_random_engine generator(std::random_device{}());
    std::normal_distribution<float> distribution(0.0, 10.0);

    for (uint32_t i=0; i<data_std_array.size(); i++)
        data_std_array[i] = distribution(generator);

    return data_std_array;
}

float* Tests::get_array(uint32_t n_pts)
{
    if (data.size() < 2*n_pts)
        data.resize(2*n_pts);

    std::default_random_engine generator(std::random_device{}());
    std::normal_distribution<float> distribution(mean, std_dev);

    for (unsigned int i=0; i<n_pts; i++) {
       data[i] = distribution(generator);
       data[i + n_pts] = distribution(generator);
    }

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

    for (unsigned int i=0; i<buffer.size(); i++)
        buffer[i] = data[i];
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