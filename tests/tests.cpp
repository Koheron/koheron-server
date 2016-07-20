/// (c) Koheron 

#include "tests.hpp"

#include <cmath>
#include <random>
#include <thread>
#include <cstring>

// http://stackoverflow.com/questions/17789928/whats-a-proper-way-of-type-punning-a-float-to-an-int-and-vice-versa
template <typename T, typename U>
inline T pseudo_cast(const U &x)
{
    T to = T(0);
    std::memcpy(&to, &x, (sizeof(T) < sizeof(U)) ? sizeof(T) : sizeof(U));
    return to;
}

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
    return abs(d - 1.428571428571428492127) < 1E-15;
}

bool Tests::set_u64(uint64_t u)
{
    return u == 2225073854759576792;
}

bool Tests::set_unsigned(uint8_t u8, uint16_t u16, uint32_t u32)
{
    return u8 == 255 && u16 == 65535 && u32 == 4294967295;
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

std::tuple<uint32_t, float, double, bool> Tests::get_tuple()
{
    return std::make_tuple(501762438, 507.3858, 926547.6468507200, true);
}

std::tuple<uint32_t, float, uint64_t, double> Tests::get_tuple2()
{
    return std::make_tuple(2, 3.14159F, 742312418498347354, 3.14159265358979323846);
}

// To check no alignement issues
std::tuple<bool, float, float, uint8_t, uint16_t> Tests::get_tuple3()
{
    return std::make_tuple(false, 3.14159F, 507.3858, 42, 6553);
}

std::array<uint32_t, 2> Tests::get_binary_tuple() {
    uint32_t v1 = 2;
    float v2 = 3.14159F;

    return {{
        v1,
        pseudo_cast<uint32_t, const float>(v2)
    }};
}

uint64_t           Tests::read_uint64()    { return (1ULL << 63);       }
int                Tests::read_int()       { return -214748364;         }
unsigned int       Tests::read_uint()      { return 301062138;          }
unsigned long      Tests::read_ulong()     { return 2048;               }
unsigned long long Tests::read_ulonglong() { return (1ULL << 63);       }
float              Tests::read_float()     { return 3.141592;           }
double             Tests::read_double()    { return 2.2250738585072009; }
bool               Tests::read_bool()      { return true;               }