#include <core/context_base.hpp>
#include <core/devices_manager.hpp>
#include <devices_table.hpp>

template<class Dev>
Dev& ContextBase::get() const {
    return dm->get<dev_id_of<Dev>>();
}

{%- for device in devices -%}
{% for object in device.objects %}
template {{ device.objects[0]['type'] }}& ContextBase::get<{{ device.objects[0]['type'] }}>() const;
{% endfor -%}
{%- endfor -%}
