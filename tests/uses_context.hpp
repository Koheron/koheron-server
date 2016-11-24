#ifndef __USES_CONTEXT_HPP__
#define __USES_CONTEXT_HPP__

#include <context.hpp>

#include "tests.hpp"

class UsesContext
{
  public:
    UsesContext(Context& ctx)
    : tests(ctx.get<2, Tests>())
    {}

  private:
    Tests& tests;
};

#endif // __USES_CONTEXT_HPP__