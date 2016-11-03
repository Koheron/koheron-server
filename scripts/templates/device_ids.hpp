
#ifndef __DEVICE_IDS_HPP__
#define __DEVICE_IDS_HPP__

namespace Op {
{% for device in devices -%}
namespace {{ device.name }} {
    {% for operation in device.operations -%}
    constexpr uint32_t {{ operation['name'] }} = {{ device.id }} + ({{ operation['id'] }} >> 16);
    {% endfor -%}
}
{% endfor %}
}

#endif // __DEVICE_IDS_HPP__