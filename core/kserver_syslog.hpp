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

namespace kserver {

class KServer;

#define FMT_BUFF_LEN 512

struct SysLog
{
    SysLog(std::shared_ptr<KServerConfig> config_, KServer *kserver_)
    : config(config_)
    , kserver(kserver_)
    {
        memset(fmt_buffer, 0, FMT_BUFF_LEN);

        if (config->syslog) {
            setlogmask(LOG_UPTO(KSERVER_SYSLOG_UPTO));
            openlog("KServer", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
        }
    }

    // This cannot be done in the destructor
    // since it is called after the "delete config"
    // at the end of the main()
    void close()
    {
        assert(config != nullptr);

        if (config->syslog) {
            print<INFO>("Close syslog ...\n");
            closelog();
        }
    }

    /// Severity of the message
    enum severity {
        PANIC,    ///< When KServer is not functionnal anymore
        CRITICAL, ///< When an error results in session crash
        ERROR,    ///< Typically when a command execution failed
        WARNING,
        INFO,
        // DEBUG, // Special print function for debug
        syslog_severity_num
    };

    template<unsigned int severity, typename... Tp>
    void print(const char *message, Tp... args);

    template<typename... Tp>
    void print_dbg(const char *message,  Tp... args)
    {
        if (config->verbose)
            printf_pack(message, args...);
    }

private:
    std::shared_ptr<KServerConfig> config;
    char fmt_buffer[FMT_BUFF_LEN];
    KServer *kserver;

    template<unsigned int severity, typename... Tp>
    void notify(const char *message, Tp... args)
    {
        static constexpr std::array<std::tuple<int, str_const>, syslog_severity_num> log_array = {{
            std::make_tuple(LOG_ALERT, str_const("KSERVER PANIC")),
            std::make_tuple(LOG_CRIT, str_const("KSERVER CRITICAL")),
            std::make_tuple(LOG_ERR, str_const("KSERVER ERROR")),
            std::make_tuple(LOG_WARNING, str_const("KSERVER WARNING")),
            std::make_tuple(LOG_NOTICE, str_const("KSERVER INFO"))
        }};

        if (severity <= WARNING) {
            auto fmt = std::string(std::get<1>(log_array[severity]).data()) + ": " + std::string(message);
            fprintf_pack(stderr, fmt.data(), args...);
        } else { // INFO
            if (config->verbose)
                printf_pack(message, args...);
        }

        if (config->syslog)
            syslog_pack(std::get<0>(log_array[severity]), message, args...);
    }

    template<unsigned int severity, typename... Tp>
    int emit_error(const char *message, Tp... args);
};

} // namespace kserver

#endif // __KSERVER_SYSLOG_HPP__
