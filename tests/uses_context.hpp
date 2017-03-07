#ifndef __USES_CONTEXT_HPP__
#define __USES_CONTEXT_HPP__

#include "context.hpp"

#include "tests.hpp"

class UsesContext
{
  public:
    UsesContext(Context& ctx_)
    : ctx(ctx_)
    , tests(ctx.get<Tests>())
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

};

#endif // __USES_CONTEXT_HPP__