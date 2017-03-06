#ifndef __USES_CONTEXT_HPP__
#define __USES_CONTEXT_HPP__

#include "context.hpp"

#include "tests.hpp"
#include "benchmarks.hpp"
#include "exception_tests.hpp"

class UsesContext
{
  public:
    UsesContext(Context& ctx_)
    : ctx(ctx_)
    , tests(ctx.get<Tests>())
    , benchmarks(ctx.get<Benchmarks>())
    , exception_tests(ctx.get<ExceptionTests>())
    {}

    bool set_float_from_tests(float f) {
        return tests.set_float(f);
    }

    bool is_myself() {
        return &ctx.get<UsesContext>() == this;
    }

  private:
    Context& ctx;

    Tests& tests;
    Benchmarks& benchmarks;
    ExceptionTests& exception_tests;
};

#endif // __USES_CONTEXT_HPP__