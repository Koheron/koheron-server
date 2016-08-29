/// {{ device.class_name|lower }}.cpp
///
/// Generated by devgen.
/// DO NOT EDIT.
///
/// (c) Koheron 

#include {{ device.class_name|lower }}.cpp

#include <core/commands.hpp>
#include <core/kserver.hpp>
#include <core/kserver_session.hpp>
#if KSERVER_HAS_DEVMEM
#include <drivers/lib/memory_manager.hpp>
#endif
namespace kserver {

#define THIS (static_cast<KS_Tests*>(this))

{% for operation in device.operations -%}
/////////////////////////////////////
// {{ operation['name'] }}

template<>
template<>
int KDevice<{{ device.class_name }}, {{ device.name }}>::
        parse_args<{{ device.class_name }}::{{ operation['name'] }}> (const Command& cmd,
                KDevice<{{ device.class_name}}, {{ device.name }}>::
                Argument<{{ device.class_name }}::{{ operation['name'] }}>& args, SessID sess_id)
{
    {{ operation | get_parser(device) }}   // TODO jinja filter
    return 0;
}

template<>
template<>
int KDevice<{{ device.class_name }}, {{ device.name }}>::
        execute_op<{{ device.class_name }}::{{ operation['name'] }}> 
        (const Argument<{{ device.class_name }}::{{ operation['name'] }}>& args, SessID sess_id)
{
    {{ operation | get_fragment(device) }}   // TODO jinja filter
}

{% endfor %}


template<>
int KDevice<{{ device.class_name }}, {{ device.name }}>::
        execute(const Command& cmd)
{
#if KSERVER_HAS_THREADS
    std::lock_guard<std::mutex> lock(THIS->mutex);
#endif

    switch(cmd.operation) {
{% for operation in device.operations -%}
      case {{ device.class_name }}::{{ operation['name'] }}: {
        Argument<{{ device.class_name }}::{{ operation['name'] }}> args;

        if (parse_arg<{{ device.class_name }}::{{ operation['name'] }}>(cmd, args, cmd.sess_id) < 0)
            return -1;

        return execute_op<{{ device.class_name }}::{{ operation['name'] }}>(args, cmd.sess_id);
      }
{% endfor %}
      case {{ device.class_name }}::{{ device.name | lower }}_op_num:
      default:
          kserver->syslog.print<SysLog::ERROR>("{{ device.class_name }}: Unknown operation\n");
          return -1;
    }
}

} // namespace kserver
