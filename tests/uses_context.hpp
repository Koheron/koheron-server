#ifndef __USES_CONTEXT_HPP__
#define __USES_CONTEXT_HPP__

#include <context.hpp>

class UsesContext
{
  public:
	UsesContext(Context& ct)
	{
		ct.dm.get<2>();
	}

};

#endif // __USES_CONTEXT_HPP__