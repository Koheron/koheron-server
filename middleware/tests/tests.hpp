/// (c) Koheron

#ifndef __TESTS_TEST_HPP__
#define __TESTS_TEST_HPP__

#include <array>
#include <vector>

#include <drivers/dev_mem.hpp> // Unused but needed for now

//> \description Tests for KServer development
class Tests
{
  public:
    Tests(Klib::DevMem& dvm_unused_);
    ~Tests();

    //> \description Open the device
    //> \io_type WRITE
    //> \param waveform_size_ Number of points to acquire
    //> \status ERROR_IF_NEG
    //> \on_error Cannot open TESTS device
    int Open(uint32_t waveform_size_);
    
    //> \description Close the device
    //> \io_type WRITE
    void Close();

    //> \io_type READ
    std::vector<float>& read();
    
    //> \io_type WRITE
    void set_mean(float mean_);

    //> \io_type WRITE
    void set_std_dev(float mean_);

    //> \io_type READ
    std::array<uint32_t, 10>& send_std_array();
    
    //> \io_type READ_ARRAY param => 2*n_pts
    float* get_array(uint32_t n_pts);
    

    // -----------------------------------------------
    // Send integers
    // -----------------------------------------------

    //> \io_type READ
    uint64_t read64();

    //> \io_type READ
    int read_int();

    //> \io_type READ
    unsigned int read_uint();

    //> \io_type READ
    unsigned long read_ulong();

    //> \io_type READ
    unsigned long long read_ulonglong();
    
    enum Status {
        CLOSED,
        OPENED,
        FAILED
    };

    //> \is_failed
    bool IsFailed() const {return status == FAILED;}

  private:
    int status;
    uint32_t waveform_size;
    
    float mean;
    float std_dev;
  
    // Data buffers
    std::vector<float> data;
    std::array<uint32_t, 10> data_std_array;
    
}; // class Tests

#endif // __TESTS_TESTS_HPP__
