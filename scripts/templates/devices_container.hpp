/// (c) Koheron

#ifndef __DEVICES_CONTAINER_HPP__
#define __DEVICES_CONTAINER_HPP__

#include <tuple>
#include <memory>

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
        return *std::get<dev - 2>(devtup).get();
    }

    template<device_t dev>
    void alloc_dev() {
        std::get<dev>(devtup) = std::make_unique<std::remove_reference_t<decltype(*std::get<dev>(devtup).get())>>(ctx);
    }

    template<std::size_t num>
    std::enable_if_t<num == 0, void>
    init_impl() {
        alloc_dev<num>();
    }

    template<std::size_t num>
    std::enable_if_t<0 < num, void>
    init_impl() {
        alloc_dev<num>();
        init_impl<num - 1>();
    }

    void init() {
        init_impl<device_num-3>();
    }


  private:
    Context& ctx;

    std::tuple<
{%- for device in devices -%}
{% if not loop.last -%}
{% for object in device.objects -%}
 std::unique_ptr<{{ device.objects[0]['type'] }}>,
{%- endfor %}
{%- else -%}
 std::unique_ptr<{{ device.objects[0]['type'] }}>
{%- endif -%}
{%- endfor -%}
> devtup;
};

} // namespace kserver

#endif // __DEVICES_CONTAINER_HPP__
