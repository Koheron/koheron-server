#include <context.hpp>
#include <core/devices_manager.hpp>

template<class Dev> constexpr device_t dev_id_of;

template<class Dev>
Dev& Context::get() {
    return dm.get<dev_id_of<Dev>>();
}

{% for device in devices -%}
template<> constexpr device_t dev_id_of<{{ device.objects[0]["type"] }}> = {{ device.id }};
{% endfor -%}

{%- for device in devices -%}
{% for object in device.objects %}
template {{ device.objects[0]['type'] }}& Context::get<{{ device.objects[0]['type'] }}>();
{% endfor -%}
{%- endfor -%}
