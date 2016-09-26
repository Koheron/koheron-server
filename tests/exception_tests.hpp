#ifndef __TESTS_EXCEPTION_TEST_HPP__
#define __TESTS_EXCEPTION_TEST_HPP__

#include <array>
#include <vector>
#include <tuple>

class ExceptionTests
{
  public:
    auto ret_type_exception() {
        return true;
    }

    std::array<float, 10>& std_array_type_exception() {
        std_array.fill(0.0);
        return std_array;
    }

    std::array<float, 10>& std_array_size_exception() {
        std_array.fill(0.0);
        return std_array;
    }

    std::vector<uint32_t>& std_vector_exception() {
        std_vector.resize(10);
        return std_vector;
    }

    auto std_tuple_exception() {
        return std::make_tuple(42U, 3.14F);
    }

  private:
    std::array<float, 10> std_array;
    std::vector<uint32_t> std_vector;
};

#endif // __TESTS_EXCEPTION_TESTS_HPP__