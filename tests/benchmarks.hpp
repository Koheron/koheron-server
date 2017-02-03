#ifndef __TESTS_BENCHMARKS_HPP__
#define __TESTS_BENCHMARKS_HPP__

#include <vector>
#include <array>
#include <cstdint>

#include "context.hpp"

class Benchmarks
{
  public:
    Benchmarks(Context& ct)
    : vector_u(16384)
    , vector_f(1048576)
    {
        array_u.fill(0);
    }

    const auto& std_vector_u32_to_client() {
        return vector_u;
    }

    const auto& std_vector_f_to_client() {
        return vector_f;
    }

    const auto& std_array_u32_to_client() {
        return array_u;
    }

    bool std_vector_u32_from_client(const std::vector<uint32_t>& vec) {
        return vec.size() == 16384;
    }

    bool std_array_u32_from_client(const std::array<uint32_t, 5000>& arr) {
        return true;
    }

  private:
    std::vector<uint32_t> vector_u;
    std::vector<float> vector_f;
    std::array<uint32_t, 16384> array_u;
};

#endif // __TESTS_BENCHMARKS_HPP__