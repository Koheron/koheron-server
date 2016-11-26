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
    : ctx(ctx_)
    {
        is_started.fill(false);
        is_starting.fill(false);
    }

    template<device_t dev>
    auto& get() {
        assert(std::get<dev - 2>(is_started));
        return *std::get<dev - 2>(devtup).get();
    }

    // TODO Detect circular dependencies

    template<device_t dev>
    int alloc() {
        if (std::get<dev - 2>(is_started))
            return 0;

        std::get<dev - 2>(devtup)
                = std::make_unique<
                    std::remove_reference_t<
                        decltype(*std::get<dev - 2>(devtup).get())
                    >
                >(ctx);
        std::get<dev - 2>(is_started) = true;
        return 0;
    }

  private:
    Context& ctx;

    std::array<bool, device_num - 2> is_started;
    std::array<bool, device_num - 2> is_starting;

    std::tuple<
{%- for device in devices -%}
{% if not loop.last -%}
 std::unique_ptr<{{ device.objects[0]['type'] }}>,
{%- else -%}
 std::unique_ptr<{{ device.objects[0]['type'] }}>
{%- endif -%}
{%- endfor -%}
> devtup;
};

} // namespace kserver

#endif // __DEVICES_CONTAINER_HPP__
