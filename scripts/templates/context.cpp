#include <context.hpp>
#include <core/devices_manager.hpp>

#include <devices_table.hpp>

template<class Dev>
Dev& Context::get() {
    return dm.get<dev_id_of<Dev>>();
}

{%- for device in devices -%}
{% for object in device.objects %}
template {{ device.objects[0]['type'] }}& Context::get<{{ device.objects[0]['type'] }}>();
{% endfor -%}
{%- endfor -%}
