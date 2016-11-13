/// {{ device.class_name|lower }}.cpp
///
/// Generated by devgen.
/// DO NOT EDIT.
///
/// (c) Koheron 

#include "{{ device.class_name|lower }}.hpp"

#include <core/commands.hpp>
#include <core/kserver.hpp>
#include <core/kserver_session.hpp>
#include <core/syslog.tpp>
#if KSERVER_HAS_DEVMEM
#include <drivers/lib/memory_manager.hpp>
#endif
namespace kserver {

#define THIS (static_cast<{{ device.class_name }}*>(this))

{% for operation in device.operations -%}
/////////////////////////////////////
// {{ operation['name'] }}

template<>
template<>
int KDevice<{{ device.class_name }}, {{ device.tag }}>::
        execute_op<{{ device.class_name }}::{{ operation['tag'] }}>(Command& cmd)
{
    {{ operation | get_parser(device) }}
    {{ operation | get_fragment(device) }}
}

{% endfor %}


template<>
int KDevice<{{ device.class_name }}, {{ device.tag }}>::
        execute(Command& cmd)
{
#if KSERVER_HAS_THREADS
    std::lock_guard<std::mutex> lock(THIS->mutex);
#endif

    switch(cmd.operation) {
{% for operation in device.operations -%}
      case {{ device.class_name }}::{{ operation['tag'] }}: {
        return execute_op<{{ device.class_name }}::{{ operation['tag'] }}>(cmd);
      }
{% endfor %}
      case {{ device.class_name }}::{{ device.tag | lower }}_op_num:
      default:
          kserver->syslog.print<SysLog::ERROR>("{{ device.class_name }}: Unknown operation\n");
          return -1;
    }
}

} // namespace kserver
