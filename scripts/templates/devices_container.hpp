/// (c) Koheron

#ifndef __DEVICES_CONTAINER_HPP__
#define __DEVICES_CONTAINER_HPP__

#include <tuple>

{% for device in devices -%}
{% for include in device.includes -%}
#include "{{ include }}"
{% endfor -%}
{% endfor %}

class Context;

namespace kserver {

class DevicesContainer
{
  public:
    DevicesContainer(Context& ctx_)
    : ctx(ctx_) {}

    template<device_t dev>
    auto& get() {
        return std::get<dev - 2>(devtup);
    }

  private:
    Context& ctx;

    std::tuple<
{%- for device in devices -%}
{% if not loop.last -%}
{% for object in device.objects -%}
 {{ device.objects[0]['type'] }},
{%- endfor %}
{%- else -%}
 {{ device.objects[0]['type'] }}
{%- endif -%}
{%- endfor -%}
> devtup = std::make_tuple(
{%- for device in devices -%}
{% if not loop.last -%}
{% for object in device.objects -%}
 {{ device.objects[0]['type'] }}(ctx),
{%- endfor %}
{%- else -%}
 {{ device.objects[0]['type'] }}(ctx)
{%- endif -%}
{%- endfor -%}
);
};

} // namespace kserver

#endif // __DEVICES_CONTAINER_HPP__