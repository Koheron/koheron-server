#ifndef __TESTS_BENCHMARKS_HPP__
#define __TESTS_BENCHMARKS_HPP__

#include <vector>
#include <array>
#include <cstdint>

class Benchmarks
{
  public:
    Benchmarks()
    : vector_u(16384)
    , vector_f(1048576)
    {
        array_u.fill(0);
    }

    const auto& std_vector_u32() {
        return vector_u;
    }

    const auto& std_vector_f() {
        return vector_f;
    }

    const auto& std_array_u32() {
        return array_u;
    }

  private:
    std::vector<uint32_t> vector_u;
    std::vector<float> vector_f;
    std::array<uint32_t, 16384> array_u;
};

#endif // __TESTS_BENCHMARKS_HPP__