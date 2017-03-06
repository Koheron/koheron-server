/// Server devices manager
///
/// (c) Koheron

#ifndef __DEVICES_MANAGER_HPP__
#define __DEVICES_MANAGER_HPP__

#include <array>
#include <memory>
#include <assert.h>
#include <thread>
#include <mutex>

#include "kdevice.hpp"

#include <devices_table.hpp>
#include <devices.hpp>

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

    template<device_id dev>
    auto& get() {
        return *std::get<dev - 2>(devtup);
    }

    template<device_id dev>
    int alloc();

  private:
    Context& ctx;

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

    template<device_id dev>
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
    std::recursive_mutex mutex;

    Context ctx;

    template<std::size_t dev> void alloc_device();

    // Start

    template<device_id... devs>
    void start(device_id dev, std::index_sequence<devs...>);

    template<device_id dev0, device_id... devs>
    std::enable_if_t<0 == sizeof...(devs) && 2 <= dev0, void>
    start_impl(device_id dev);

    template<device_id dev0, device_id... devs>
    std::enable_if_t<0 < sizeof...(devs) && 2 <= dev0, void>
    start_impl(device_id dev);

    // Execute

    template<device_id... devs>
    int execute_dev(KDeviceAbstract *dev_abs, Command& cmd,
                    std::index_sequence<devs...>);

    template<device_id dev0, device_id... devs>
    std::enable_if_t<0 == sizeof...(devs) && 2 <= dev0, int>
    execute_dev_impl(KDeviceAbstract *dev_abs, Command& cmd);

    template<device_id dev0, device_id... devs>
    std::enable_if_t<0 < sizeof...(devs) && 2 <= dev0, int>
    execute_dev_impl(KDeviceAbstract *dev_abs, Command& cmd);
};

} // namespace kserver

#endif // __DEVICES_MANAGER_HPP__
