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

    int Open(uint32_t waveform_size_);

    bool set_float(float f);

    // Send arrays
    std::vector<float>& send_std_vector();
    std::array<float, 10>& send_std_array();

    #pragma tcp-server read_array 2*arg{n_pts}
    float* send_c_array1(uint32_t n_pts);

    #pragma tcp-server read_array this{data.size()}
    float* get_array_bis();

    // Receive array
    #pragma tcp-server write_array arg{data} arg{len}
    bool set_buffer(const uint32_t *data, uint32_t len);

    // Send strings
    const char* get_cstr();

    std::tuple<int, float, double>
    get_tuple()
    {
        return std::make_tuple(2, 3.14159F, 2345.6);
    }

    // Send integers
    uint64_t read64();
    int read_int();
    unsigned int read_uint();
    unsigned long read_ulong();
    unsigned long long read_ulonglong();
    float read_float();
    bool read_bool();

    enum Status {
        CLOSED,
        OPENED,
        FAILED
    };

    #pragma tcp-server is_failed
    bool IsFailed() const {return status == FAILED;}


    std::vector<float> data;
  private:
    int status;
    uint32_t waveform_size;
    float mean;
    float std_dev;

    std::vector<uint32_t> buffer;
    std::array<float, 10> data_std_array;
}; // class Tests

#endif // __TESTS_TESTS_HPP__
