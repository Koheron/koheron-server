#ifndef __TESTS_TEST_HPP__
#define __TESTS_TEST_HPP__

#include <array>
#include <vector>
#include <tuple>
#include <string>

constexpr size_t calc_array_length(size_t n_bits) {
    return 1 << n_bits;
}

#define HALF_ARRAY_LEN 24

class Tests
{
  public:
    Tests() 
    : data(0)
    , buffer(0)
    {}

    bool rcv_many_params(uint32_t u1, uint32_t u2, float f, bool b);
    bool set_float(float f);
    bool set_double(double d);
    bool set_u64(uint64_t u);
    bool set_i64(int64_t i);
    bool set_unsigned(uint8_t u8, uint16_t u16, uint32_t u32);
    bool set_signed(int8_t i8, int16_t i16, int32_t i32);

    // Send arrays
    std::vector<float>& send_std_vector();
    std::vector<uint32_t>& send_std_vector2();
    std::array<float, 10>& send_std_array();

    auto& send_std_array2(uint32_t mul) {
        for (uint32_t i=0; i<data_std_array2.size(); i++)
            data_std_array2[i] = mul * i;

        return data_std_array2;
    }

    std::array<uint32_t, 2 * HALF_ARRAY_LEN>& send_std_array3(uint32_t add) {
        for (uint32_t i=0; i<data_std_array3.size(); i++)
            data_std_array3[i] = add + i;

        return data_std_array3;
    }

    bool rcv_std_array(uint32_t u, float f, const std::array<uint32_t, 8192>& arr, double d, int32_t i);
    bool rcv_std_array2(const std::array<float, 8192>& arr);
    bool rcv_std_array3(const std::array<double, 8192>& arr);
    bool rcv_std_array4(const std::array<uint32_t, calc_array_length(10)>& arr);

    // Receive vector
    bool rcv_std_vector(const std::vector<uint32_t>& vec);
    bool rcv_std_vector1(uint32_t u, float f, const std::vector<double>& vec);
    bool rcv_std_vector2(uint32_t u, float f, const std::vector<float>& vec, double d, int32_t i);
    bool rcv_std_vector3(const std::array<uint32_t, 8192>& arr, const std::vector<float>& vec, double d, int32_t i);
    bool rcv_std_vector4(const std::vector<float>& vec, double d, int32_t i, const std::array<uint32_t, 8192>& arr);
    bool rcv_std_vector5(const std::vector<float>& vec1, double d, int32_t i, const std::vector<float>& vec2);

    // Receive string
    bool rcv_std_string(const std::string& str);
    bool rcv_std_string1(const std::string& str);
    bool rcv_std_string2(const std::string& str, const std::vector<float>& vec, double d, int32_t i);
    bool rcv_std_string3(const std::vector<float>& vec, double d, int32_t i, const std::string& str, const std::array<uint32_t, 8192>& arr);

    // Send string
    const char* get_cstr();
    std::string get_std_string();
    std::string get_json();
    std::string get_json2();

    // Send tuple
    auto get_tuple() {
        return std::make_tuple(501762438, 507.3858, 926547.6468507200, true);
    }

    std::tuple<uint32_t, float, uint64_t, double, int64_t> get_tuple2();
    std::tuple<bool, float, float, uint8_t, uint16_t> get_tuple3();
    std::tuple<int8_t, int8_t, int16_t, int16_t, int32_t, int32_t> get_tuple4();

    // Send numbers
    uint64_t read_uint64();
    int32_t read_int();
    uint32_t read_uint();
    uint32_t read_ulong();
    uint64_t read_ulonglong();
    float read_float();
    double read_double();
    bool read_bool();

    std::vector<float> data;
    std::vector<uint32_t> data_u;

  private:
    std::vector<uint32_t> buffer;
    std::array<float, 10> data_std_array;
    std::array<uint32_t, 512> data_std_array2;
    std::array<uint32_t, 2 * HALF_ARRAY_LEN> data_std_array3;
};

#endif // __TESTS_TESTS_HPP__
