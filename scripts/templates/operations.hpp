
#ifndef __OPERATIONS_HPP__
#define __OPERATIONS_HPP__

#include <tuple>
#include <type_traits>

{% for device in devices -%}
{% for include in device.includes -%}
#include <{{ include }}>
{% endfor -%}
{% endfor %}

namespace op {
{% for device in devices -%}
namespace {{ device.name }} {
    {% for operation in device.operations -%}
    constexpr uint32_t {{ operation['name'] }} = ({{ device.id }} << 16) + {{ operation['id'] }};
    {% endfor -%}
}
{% endfor %}
}

{% for device in devices -%}
    {% for operation in device.operations -%}

    template<>
    struct arg_types<op::{{ device.name }}::{{ operation['name'] }}> {
        using type = std::tuple<
            {%- for arg in operation['arguments'] -%}
                {%- if not loop.last -%}
                    {{ arg['type'] }},
                {%- else -%}
                    {{ arg['type'] }}
                {%- endif -%}
            {%- endfor -%}
        >;
    };

    template<>
    struct ret_type<op::{{ device.name }}::{{ operation['name'] }}> {
        using type = std::decay_t<{{ device.name | get_exact_ret_type(operation) }}>;
    };

    {% endfor -%}
{% endfor %}

#endif // __OPERATIONS_HPP__