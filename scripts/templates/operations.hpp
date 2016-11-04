
#ifndef __OPERATIONS_HPP__
#define __OPERATIONS_HPP__

namespace op {
{% for device in devices -%}
namespace {{ device.name }} {
    {% for operation in device.operations -%}
    constexpr uint32_t {{ operation['name'] }} = ({{ device.id }} << 16) + {{ operation['id'] }};
    {% endfor -%}
}
{% endfor %}
}

#endif // __OPERATIONS_HPP__