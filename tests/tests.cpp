/// (c) Koheron 

#include "tests.hpp"

#include <cmath>
#include <cstring>
#include <limits>

bool Tests::rcv_many_params(uint32_t u1, uint32_t u2, float f, bool b)
{
    return u1 == 429496729 && u2 == 2048 && trunc(f * 1E6) == 3140000 && b;
}

bool Tests::set_float(float f)
{
    return f == 12.5;
}

bool Tests::set_double(double d)
{
    return fabs(d - 1.428571428571428492127) <= std::numeric_limits<double>::epsilon();
}

bool Tests::set_u64(uint64_t u)
{
    return u == 2225073854759576792;
}

bool Tests::set_i64(int64_t i)
{
    return i == -9223372036854775805;
}


bool Tests::set_unsigned(uint8_t u8, uint16_t u16, uint32_t u32)
{
    return u8 == 255 && u16 == 65535 && u32 == 4294967295;
}

bool Tests::set_signed(int8_t i8, int16_t i16, int32_t i32)
{
    return i8 == -125 && i16 == -32764 && i32 == -2147483645;
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
    if (len != 10) return false;

    for (unsigned int i=0; i<len; i++)
        if (data[i] != i*i)
            return false;

    return true;
}

bool Tests::rcv_std_array(uint32_t u, float f, const std::array<uint32_t, 8192>& arr, double d, int32_t i)
{
    if (u != 4223453) return false;
    if (fabs(f - 3.141592) > std::numeric_limits<float>::epsilon()) return false;
    if (fabs(d - 2.654798454646) > std::numeric_limits<double>::epsilon()) return false;
    if (i != -56789) return false;

    for (unsigned int i=0; i<8192; i++)
        if (arr[i] != i) return false;

    return true;
}

bool Tests::rcv_std_array2(const std::array<float, 8192>& arr)
{
    for (unsigned int i=0; i<8192; i++)
        if (fabs(arr[i] - log(static_cast<float>(i + 1))) > std::numeric_limits<float>::round_error())
            return false;

    return true;
}

bool Tests::rcv_std_array3(const std::array<double, 8192>& arr)
{
    for (unsigned int i=0; i<8192; i++)
        if (fabs(arr[i] - sin(static_cast<double>(i))) > std::numeric_limits<double>::epsilon())
            return false;

    return true;
}

bool Tests::rcv_std_vector(const std::vector<uint32_t>& vec)
{
    if (vec.size() != 8192) return false;

    for (unsigned int i=0; i<vec.size(); i++)
        if (vec[i] != i) return false;

    return true;
}

bool Tests::rcv_std_vector1(uint32_t u, float f, const std::vector<double>& vec)
{
    if (u != 4223453) return false;
    if (fabs(f - 3.141592) > std::numeric_limits<float>::epsilon()) return false;

    if (vec.size() != 8192) return false;

    for (unsigned int i=0; i<vec.size(); i++)
        if (fabs(vec[i] - sin(static_cast<double>(i))) > std::numeric_limits<double>::epsilon())
            return false;

    return true;
}

const char* Tests::get_cstr()
{
    return "Hello !";
}

std::string Tests::get_std_string()
{
    return "Hello World !";
}

std::string Tests::get_json()
{
    return "{\"date\":\"20/07/2016\",\"machine\":\"PC-3\",\"time\":\"18:16:13\",\"user\":\"thomas\",\"version\":\"0691eed\"}";
}

std::string Tests::get_json2()
{
    return "{\"firstName\":\"John\",\"lastName\":\"Smith\",\"age\":25,\"phoneNumber\":[{\"type\":\"home\",\"number\":\"212 555-1234\"},{\"type\":\"fax\",\"number\":\"646 555-4567\"}]}";
}

std::tuple<uint32_t, float, uint64_t, double, int64_t> Tests::get_tuple2()
{
    return std::make_tuple(2, 3.14159F, 742312418498347354,
                           3.14159265358979323846, -9223372036854775807);
}

// To check no alignement issues
std::tuple<bool, float, float, uint8_t, uint16_t> Tests::get_tuple3()
{
    return std::make_tuple(false, 3.14159F, 507.3858, 42, 6553);
}

std::tuple<int8_t, int8_t, int16_t, int16_t, int32_t, int32_t> Tests::get_tuple4()
{
    return std::make_tuple(-127, 127, -32767, 32767, -2147483647, 2147483647);
}

uint64_t      Tests::read_uint64()    { return (1ULL << 63);       }
int           Tests::read_int()       { return -214748364;         }
unsigned int  Tests::read_uint()      { return 301062138;          }
uint32_t      Tests::read_ulong()     { return 2048;               }
uint64_t      Tests::read_ulonglong() { return (1ULL << 63);       }
float         Tests::read_float()     { return 3.141592;           }
double        Tests::read_double()    { return 2.2250738585072009; }
bool          Tests::read_bool()      { return true;               }
