/// (c) Koheron

#include "tests.hpp"

#include <cmath>
#include <cstring>
#include <limits>

bool Tests::rcv_many_params(uint32_t u1, uint32_t u2, float f, bool b) {
    return u1 == 429496729 && u2 == 2048 && trunc(f * 1E6) == 3140000 && b;
}

bool Tests::set_float(float f) {
    return f == 12.5;
}

bool Tests::set_double(double d) {
    return fabs(d - 1.428571428571428492127) <= std::numeric_limits<double>::epsilon();
}

bool Tests::set_u64(uint64_t u) {
    return u == 2225073854759576792;
}

bool Tests::set_i64(int64_t i) {
    return i == -9223372036854775805;
}

bool Tests::set_unsigned(uint8_t u8, uint16_t u16, uint32_t u32) {
    return u8 == 255 && u16 == 65535 && u32 == 4294967295;
}

bool Tests::set_signed(int8_t i8, int16_t i16, int32_t i32) {
    return i8 == -125 && i16 == -32764 && i32 == -2147483645;
}

std::vector<float>& Tests::send_std_vector() {
    data.resize(10);

    for (unsigned int i=0; i<data.size(); i++)
        data[i] = i*i*i;

    return data;
}

std::vector<uint32_t>& Tests::send_std_vector2() {
    data_u.resize(20);

    for (unsigned int i=0; i<data_u.size(); i++)
        data_u[i] = i*i;

    return data_u;
}

std::vector<int32_t>& Tests::send_std_vector3() {
    data_i.resize(20);

    for (unsigned int i=0; i<data_i.size(); i++)
        data_i[i] = -i*i;

    return data_i;
}

std::array<float, 10>& Tests::send_std_array() {
    for (uint32_t i=0; i<data_std_array.size(); i++)
        data_std_array[i] = i*i;

    return data_std_array;
}

bool Tests::rcv_std_array(uint32_t u, float f, const std::array<uint32_t, 8192>& arr, double d, int32_t i) {
    if (u != 4223453) return false;
    if (fabs(f - 3.141592) > std::numeric_limits<float>::epsilon()) return false;
    if (fabs(d - 2.654798454646) > std::numeric_limits<double>::epsilon()) return false;
    if (i != -56789) return false;

    for (unsigned int i=0; i<8192; i++)
        if (arr[i] != i) return false;

    return true;
}

bool Tests::rcv_std_array2(const std::array<float, 8192>& arr) {
    for (unsigned int i=0; i<8192; i++)
        if (fabs(arr[i] - log(static_cast<float>(i + 1))) > std::numeric_limits<float>::round_error())
            return false;

    return true;
}

bool Tests::rcv_std_array3(const std::array<double, 8192>& arr) {
    for (unsigned int i=0; i<8192; i++)
        if (fabs(arr[i] - sin(static_cast<double>(i))) > std::numeric_limits<double>::epsilon())
            return false;

    return true;
}

bool Tests::rcv_std_array4(const std::array<uint32_t, calc_array_length(10)>& arr) {
    for (unsigned int i=0; i<calc_array_length(10); i++)
        if (arr[i] != i) return false;

    return true;
}

bool Tests::rcv_std_vector(const std::vector<uint32_t>& vec) {
    if (vec.size() != 8192) return false;

    for (unsigned int i=0; i<vec.size(); i++)
        if (vec[i] != i) return false;

    return true;
}

bool Tests::rcv_std_vector1(uint32_t u, float f, const std::vector<double>& vec) {
    if (u != 4223453) return false;
    if (fabs(f - 3.141592) > std::numeric_limits<float>::epsilon()) return false;

    if (vec.size() != 8192) return false;

    for (unsigned int i=0; i<vec.size(); i++)
        if (fabs(vec[i] - sin(static_cast<double>(i))) > std::numeric_limits<double>::epsilon())
            return false;

    return true;
}

bool Tests::rcv_std_vector2(uint32_t u, float f, const std::vector<float>& vec, double d, int32_t i) {
    if (u != 4223453) return false;
    if (fabs(f - 3.141592) > std::numeric_limits<float>::epsilon()) return false;

    if (vec.size() != 8192) return false;

    for (unsigned int i=0; i<vec.size(); i++)
        if (fabs(vec[i] - log(static_cast<float>(i + 1))) > std::numeric_limits<float>::round_error())
            return false;

    if (fabs(d - 2.654798454646) > std::numeric_limits<double>::epsilon()) return false;
    if (i != -56789) return false;

    return true;
}

bool Tests::rcv_std_vector3(const std::array<uint32_t, 8192>& arr,
                            const std::vector<float>& vec, double d, int32_t i) {
    if (fabs(d - 2.654798454646) > std::numeric_limits<double>::epsilon()) return false;
    if (i != -56789) return false;

    for (unsigned int i=0; i<8192; i++)
        if (arr[i] != i) return false;

    if (vec.size() != 8192) return false;

    for (unsigned int i=0; i<vec.size(); i++)
        if (fabs(vec[i] - log(static_cast<float>(i + 1))) > std::numeric_limits<float>::round_error())
            return false;

    return true;
}

bool Tests::rcv_std_vector4(const std::vector<float>& vec, double d, int32_t i,
                            const std::array<uint32_t, 8192>& arr) {
    if (fabs(d - 2.654798454646) > std::numeric_limits<double>::epsilon()) return false;
    if (i != -56789) return false;
    if (vec.size() != 8192) return false;

    for (unsigned int i=0; i<vec.size(); i++)
        if (fabs(vec[i] - cos(static_cast<float>(i))) > std::numeric_limits<float>::epsilon())
            return false;

    for (unsigned int i=0; i<8192; i++)
        if (arr[i] != i * i) return false;

    return true;
}

bool Tests::rcv_std_vector5(const std::vector<float>& vec1, double d, int32_t i, const std::vector<float>& vec2) {
    if (fabs(d - 0.4232747024077716) > std::numeric_limits<double>::epsilon()) return false;
    if (i != 35591508) return false;

    if (vec1.size() != 8192) return false;
    if (vec2.size() != 16384) return false;

    for (unsigned int i=0; i<vec1.size(); i++)
        if (fabs(vec1[i] - tanh(static_cast<float>(i))) > std::numeric_limits<float>::epsilon())
            return false;

    for (unsigned int i=0; i<vec2.size(); i++)
        if (fabs(vec2[i] - log2(static_cast<float>(i + 1))) > std::numeric_limits<float>::round_error())
            return false;

    return true;
}

bool Tests::rcv_std_string(const std::string& str) {
    return str == "Hello World";
}

bool Tests::rcv_std_string1(const std::string& str) {
    return str == "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer nec odio. Praesent libero. Sed cursus ante dapibus diam. Sed nisi. Nulla quis sem at nibh elementum imperdiet. Duis sagittis ipsum. Praesent mauris. Fusce nec tellus sed augue semper porta. Mauris massa. Vestibulum lacinia arcu eget nulla. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Curabitur sodales ligula in libero. Sed dignissim lacinia nunc.";
}

bool Tests::rcv_std_string2(const std::string& str, const std::vector<float>& vec, double d, int32_t i) {
    if (str != "At vero eos et accusamus et iusto odio dignissimos ducimus, qui blanditiis praesentium voluptatum deleniti atque corrupti")
        return false;

    if (vec.size() != 8192)
        return false;

    for (unsigned int i=0; i<vec.size(); i++)
        if (fabs(vec[i] - sin(static_cast<float>(i))) > std::numeric_limits<float>::epsilon())
            return false;

    if (fabs(d - 0.80773675317454) > std::numeric_limits<double>::epsilon())
        return false;

    if (i != -361148845)
        return false;

    return true;
}

bool Tests::rcv_std_string3(const std::vector<float>& vec, double d, int32_t i, const std::string& str, const std::array<uint32_t, 8192>& arr)
{
    if (vec.size() != 8192)
        return false;

    for (unsigned int i=0; i<vec.size(); i++)
        if (fabs(vec[i] - sqrt(static_cast<float>(i))) > std::numeric_limits<float>::round_error())
            return false;

    if (fabs(d - 0.4741953746153866) > std::numeric_limits<double>::epsilon())
        return false;

    if (i != -6093602)
        return false;

    if (str != "Erbium is a rare-earth element that, when excited, emits light around 1.54 micrometers - the low-loss wavelength for optical fibers used in DWDM. A weak signal enters the erbium-doped fiber, into which light at 980nm or 1480nm is injected using a pump laser. This injected light stimulates the erbium atoms to release their stored energy as additional 1550nm light. As this process continues down the fiber, the signal grows stronger. The spontaneous emissions in the EDFA also add noise to the signal; this determines the noise figure of an EDFA.")
        return false;

    for (unsigned int i=0; i<8192; i++)
        if (arr[i] != i * i) return false;

    return true;
}

const char* Tests::get_cstr() {
    return "Hello !";
}

std::string Tests::get_std_string() {
    return "Hello World !";
}

std::string Tests::get_json() {
    return "{\"date\":\"20/07/2016\",\"machine\":\"PC-3\",\"time\":\"18:16:13\",\"user\":\"thomas\",\"version\":\"0691eed\"}";
}

std::string Tests::get_json2() {
    return "{\"firstName\":\"John\",\"lastName\":\"Smith\",\"age\":25,\"phoneNumber\":[{\"type\":\"home\",\"number\":\"212 555-1234\"},{\"type\":\"fax\",\"number\":\"646 555-4567\"}]}";
}

std::tuple<uint32_t, float, uint64_t, double, int64_t> Tests::get_tuple2() {
    return std::make_tuple(2, 3.14159F, 742312418498347354,
                           3.14159265358979323846, -9223372036854775807);
}

// To check no alignement issues
std::tuple<bool, float, float, uint8_t, uint16_t> Tests::get_tuple3() {
    return std::make_tuple(false, 3.14159F, 507.3858, 42, 6553);
}

std::tuple<int8_t, int8_t, int16_t, int16_t, int32_t, int32_t> Tests::get_tuple4() {
    return std::make_tuple(-127, 127, -32767, 32767, -2147483647, 2147483647);
}

uint64_t      Tests::read_uint64()    { return (1ULL << 63);       }
int32_t       Tests::read_int()       { return -214748364;         }
uint32_t      Tests::read_uint()      { return 301062138;          }
uint32_t      Tests::read_ulong()     { return 2048;               }
uint64_t      Tests::read_ulonglong() { return (1ULL << 63);       }
float         Tests::read_float()     { return 3.141592;           }
double        Tests::read_double()    { return 2.2250738585072009; }
bool          Tests::read_bool()      { return true;               }
