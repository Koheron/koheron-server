/// Server system log
///
/// (c) Koheron

#ifndef __KSERVER_SYSLOG_HPP__
#define __KSERVER_SYSLOG_HPP__

#include "config.hpp"

#include <memory>
#include <string>
#include <cstring>
#include <tuple>

#include <syslog.h>

#include "string_utils.hpp"
#include "pubsub.hpp"

/// Severity of the message
enum severity {
    PANIC,    ///< When KServer is not functionnal anymore
    CRITICAL, ///< When an error results in session crash
    ERROR,    ///< Typically when a command execution failed
    WARNING,
    INFO,
    DEBUG, // Special print function for debug
    syslog_severity_num
};

namespace kserver {

class KServer;

static constexpr auto log_array = kserver::make_array(
    std::make_tuple(LOG_ALERT, str_const("KSERVER PANIC")),
    std::make_tuple(LOG_CRIT, str_const("KSERVER CRITICAL")),
    std::make_tuple(LOG_ERR, str_const("KSERVER ERROR")),
    std::make_tuple(LOG_WARNING, str_const("KSERVER WARNING")),
    std::make_tuple(LOG_NOTICE, str_const("KSERVER INFO")),
    std::make_tuple(LOG_DEBUG, str_const("KSERVER DEBUG"))
);

template<unsigned int severity>
constexpr int to_priority = std::get<0>(std::get<severity>(log_array));

template<unsigned int severity>
constexpr str_const severity_msg = std::get<1>(std::get<severity>(log_array));

struct SysLog
{
    template<unsigned int severity, typename... Args>
    void print(const char *msg, Args&&... args);

    template<uint16_t channel, uint16_t event, typename... Args>
    int notify(const char *msg, Args&&... args);

  private:
    std::shared_ptr<KServerConfig> config;
    PubSub pubsub;

  private:
    SysLog(std::shared_ptr<KServerConfig> config_,
           SignalHandler& sig_handler_,
           SessionManager& sess_manager_)
    : config(config_)
    , pubsub(sess_manager_, sig_handler_)
    {
        if (config->syslog) {
            setlogmask(LOG_UPTO(KSERVER_SYSLOG_UPTO));
            openlog("KServer", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
        }
    }

    // This cannot be done in the destructor
    // since it is called after the "delete config"
    // at the end of the main()
    void close() {
        assert(config != nullptr);

        if (config->syslog) {
            print<INFO>("Close syslog ...\n");
            closelog();
        }
    }

    // High severity (Panic, ..., Warning) => stderr
    template<unsigned int severity, typename... Args>
    std::enable_if_t< severity <= WARNING, void >
    print_msg(const char *message, Args... args) {
        kserver::fprintf(stderr, (severity_msg<severity>.to_string() + ": " + std::string(message)).c_str(),
                         std::forward<Args>(args)...);
    }

    // Low severity (Info, Debug) => stdout if verbose
    template<unsigned int severity, typename... Args>
    std::enable_if_t< severity >= INFO, void >
    print_msg(const char *message, Args&&... args) {
        if (config->verbose)
            kserver::printf(message, std::forward<Args>(args)...);
    }

    template<unsigned int severity, typename... Args>
    std::enable_if_t< severity <= INFO, void >
    call_syslog(const char *message, Args&&... args) {
        if (config->syslog)
            syslog<to_priority<severity>>(message, std::forward<Args>(args)...);
    }

    // We don't send debug messages to the system log
    template<unsigned int severity, typename... Args>
    std::enable_if_t< severity >= DEBUG, int >
    call_syslog(const char *message, Args&&... args) {
        return 0;
    }

    template<unsigned int severity, typename... Args>
    std::enable_if_t< severity <= INFO, int >
    emit_error(const char *message, Args&&... args) {
        return notify<PubSub::SYSLOG_CHANNEL, severity>(message, std::forward<Args>(args)...);
    }

    template<unsigned int severity, typename... Args>
    std::enable_if_t< severity >= DEBUG, int >
    emit_error(const char *message, Args... args) {
        return 0;
    }

friend class KServer;
friend class SessionManager;
};

template<unsigned int severity, typename... Args>
void SysLog::print(const char *msg, Args&&... args)
{
    static_assert(severity <= syslog_severity_num, "Invalid logging level");

    print_msg<severity>(msg, std::forward<Args>(args)...);
    call_syslog<severity>(msg, std::forward<Args>(args)...);
    emit_error<severity>(msg, std::forward<Args>(args)...);
}

} // namespace kserver

#endif // __KSERVER_SYSLOG_HPP__
