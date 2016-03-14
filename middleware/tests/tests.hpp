#ifndef __TESTS_TEST_HPP__
#define __TESTS_TEST_HPP__

#include <array>
#include <vector>

#include <drivers/dev_mem.hpp> 

class Tests
{
  public:
    Tests(Klib::DevMem& dvm_unused_);
    ~Tests();

    int Open(uint32_t waveform_size_);
    
    #pragma tcp-server exclude
    void Close();

    void set_mean(float mean_);
    void set_std_dev(float mean_);

    // Send arrays
    std::vector<float>& read();
    std::array<uint32_t, 10>& send_std_array();

    #pragma tcp-server read_array 2*arg{n_pts}
    float* get_array(uint32_t n_pts);

    #pragma tcp-server read_array this{data.size()}
    float* get_array_bis();

    // Receive array
    #pragma tcp-server write_array arg{data} arg{len}
    void set_buffer(const uint32_t *data, uint32_t len);

    // Send strings
    const char* get_cstr();

    // Send integers
    uint64_t read64();
    int read_int();
    unsigned int read_uint();
    unsigned long read_ulong();
    unsigned long long read_ulonglong();

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
    std::array<uint32_t, 10> data_std_array;
}; // class Tests

#endif // __TESTS_TESTS_HPP__
