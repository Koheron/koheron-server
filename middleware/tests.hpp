#ifndef __TESTS_TEST_HPP__
#define __TESTS_TEST_HPP__

#include <array>
#include <vector>
#include <tuple>

#include <drivers/lib/dev_mem.hpp> 

class Tests
{
  public:
    Tests(Klib::DevMem& dvm_unused_);

    bool rcv_many_params(uint32_t u1, uint32_t u2, float f, bool b);
    bool set_float(float f);

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
    std::tuple<int, float, double> get_tuple();

    // Send numbers
    uint64_t read64();
    int read_int();
    unsigned int read_uint();
    unsigned long read_ulong();
    unsigned long long read_ulonglong();
    float read_float();
    bool read_bool();

    std::vector<float> data;

  private:
    std::vector<uint32_t> buffer;
    std::array<float, 10> data_std_array;
}; // class Tests

#endif // __TESTS_TESTS_HPP__
