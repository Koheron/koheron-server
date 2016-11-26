/// (c) Koheron

#ifndef __DEVICES_CONTAINER_HPP__
#define __DEVICES_CONTAINER_HPP__

#include <tuple>
#include <memory>

#include <core/syslog.hpp>

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
    DevicesContainer(Context& ctx_, SysLog& syslog_)
    : ctx(ctx_)
    , syslog(syslog_)
    {
        is_started.fill(false);
        is_starting.fill(false);
    }

    template<device_t dev>
    auto& get() {
        return *std::get<dev - 2>(devtup).get();
    }

    template<device_t dev>
    int alloc() {
        if (std::get<dev - 2>(is_started))
            return 0;

        if (std::get<dev - 2>(is_starting)) {
            syslog.print<CRITICAL>(
                "Circular dependency detected while initializing device [%u] %s\n",
                dev, std::get<dev>(devices_names).data());

            return -1;
        }

        std::get<dev - 2>(is_starting) = true;

        std::get<dev - 2>(devtup)
                = std::make_unique<
                    std::remove_reference_t<
                        decltype(*std::get<dev - 2>(devtup).get())
                    >
                >(ctx);

        std::get<dev - 2>(is_starting) = false;
        std::get<dev - 2>(is_started) = true;
        return 0;
    }

  private:
    Context& ctx;
    SysLog& syslog;

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
