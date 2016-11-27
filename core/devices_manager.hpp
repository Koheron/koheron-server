/// Server devices manager
///
/// (c) Koheron

#ifndef __DEVICES_MANAGER_HPP__
#define __DEVICES_MANAGER_HPP__

#include <array>
#include <memory>
#include <assert.h>

#include "kdevice.hpp"

#include <devices_table.hpp>
#include <devices.hpp>

#if KSERVER_HAS_THREADS
#  include <thread>
#  include <mutex>
#endif

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
        return *std::get<dev - 2>(devtup);
    }

    template<device_t dev>
    int alloc();

  private:
    Context& ctx;
    SysLog& syslog;

    std::array<bool, device_num - 2> is_started;
    std::array<bool, device_num - 2> is_starting;

    devices_tuple_t devtup;
};

class KServer;
struct Command;

class DeviceManager
{
  public:
    DeviceManager(KServer *kserver_);

    int init();
    int execute(Command &cmd);

    template<device_t dev>
    auto& get() {
        if (! std::get<dev - 2>(is_started))
            alloc_device<dev>();
        return dev_cont.get<dev>();
    }

  private:
    // Store devices (except KServer) as unique_ptr
    std::array<std::unique_ptr<KDeviceAbstract>, device_num - 2> device_list;
    KServer *kserver;
    DevicesContainer dev_cont;
    std::array<bool, device_num - 2> is_started;

#if KSERVER_HAS_THREADS
    std::recursive_mutex mutex;
#endif

    template<std::size_t dev> void alloc_device();

    // Start

    template<device_t... devs>
    void start(device_t dev, std::index_sequence<devs...>);

    template<device_t dev0, device_t... devs>
    std::enable_if_t<0 == sizeof...(devs) && 2 <= dev0, void>
    start_impl(device_t dev);

    template<device_t dev0, device_t... devs>
    std::enable_if_t<0 < sizeof...(devs) && 2 <= dev0, void>
    start_impl(device_t dev);

    // Execute

    template<device_t... devs>
    int execute_dev(KDeviceAbstract *dev_abs, Command& cmd,
                    std::index_sequence<devs...>);

    template<device_t dev0, device_t... devs>
    std::enable_if_t<0 == sizeof...(devs) && 2 <= dev0, int>
    execute_dev_impl(KDeviceAbstract *dev_abs, Command& cmd);

    template<device_t dev0, device_t... devs>
    std::enable_if_t<0 < sizeof...(devs) && 2 <= dev0, int>
    execute_dev_impl(KDeviceAbstract *dev_abs, Command& cmd);
};

} // namespace kserver

#endif // __DEVICES_MANAGER_HPP__
