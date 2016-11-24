#include <context.hpp>
#include <core/devices_manager.hpp>

template<std::size_t dev, class Dev>
Dev& Context::get() {
    return dm.get<dev>();
}

{%- for device in devices -%}
{% for object in device.objects %}
template {{ device.objects[0]['type'] }}& Context::get<{{device.tag}}, {{ device.objects[0]['type'] }}>();
{% endfor -%}
{%- endfor -%}
