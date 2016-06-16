#ifndef __TESTS_TEST_HPP__
#define __TESTS_TEST_HPP__

#include <array>
#include <vector>
#include <tuple>

#if KSERVER_HAS_DEVMEM
#include <drivers/lib/dev_mem.hpp>
#endif

class Tests
{
  public:
#if KSERVER_HAS_DEVMEM
    Tests(
        Klib::DevMem& dvm_unused_)
    : data(0)
    , buffer(0)
    {}
#else
    Tests() 
    : data(0)
    , buffer(0)
    {}
#endif

    bool rcv_many_params(uint32_t u1, uint32_t u2, float f, bool b);
    bool set_float(float f);
    bool set_double(double d);
    bool set_u64(uint64_t u);

    // Send arrays
    std::vector<float>& send_std_vector();
    std::array<float, 10>& send_std_array();

    #pragma tcp-server read_array 2*arg{n_pts}
    float* send_c_array1(uint32_t n_pts);

    #pragma tcp-server read_array this{data.size()}
    float* send_c_array2();

    // Receive array
    #pragma tcp-server write_array arg{data} arg{len}
    bool set_buffer(const uint32_t *data, uint32_t len);

    // Send string
    const char* get_cstr();

    // Send tuple
    std::tuple<uint32_t, float, double> get_tuple();
    std::tuple<uint32_t, float, uint64_t, double> get_tuple2();
    std::array<uint32_t, 2> get_binary_tuple();

    // Send numbers
    uint64_t read_uint64();
    int read_int();
    unsigned int read_uint();
    unsigned long read_ulong();
    unsigned long long read_ulonglong();
    float read_float();
    double read_double();
    bool read_bool();

    std::vector<float> data;

  private:
    std::vector<uint32_t> buffer;
    std::array<float, 10> data_std_array;
}; // class Tests

#endif // __TESTS_TESTS_HPP__
