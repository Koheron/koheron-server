/// Tests for tuple tranfers
/// (c) Koheron

#ifndef __TUPLE_TESTS_HPP__
#define __TUPLE_TESTS_HPP__

#include <tuple>

#include <drivers/lib/dev_mem.hpp> // Unused but needed for now

class TupleTests
{
  public:
    TupleTests(Klib::DevMem& dvm_unused_) {}

    std::tuple<int,
               float, 
               double>
    get_tuple()
    {
        return std::make_tuple(2, 3.14159F, 2345.678);
    }
};

#endif // __TUPLE_TESTS_HPP__
