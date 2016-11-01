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
    void close() {
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
        DEBUG, // Special print function for debug
        syslog_severity_num
    };

    template<unsigned int severity, typename... Tp>
    void print(const std::string& msg, Tp... args);

  private:
    std::shared_ptr<KServerConfig> config;
    char fmt_buffer[FMT_BUFF_LEN];
    KServer *kserver;

  private:
    // High severity (Panic, ..., Warning)
    template<unsigned int severity, typename... Tp>
    typename std::enable_if_t< severity <= WARNING, void >
    print_msg(const str_const& severity_desc, const std::string& message, Tp... args) {
        fprintf(stderr, severity_desc.to_string() + ": " + message, args...);
    }

    // Low severity (Info, Debug)
    template<unsigned int severity, typename... Tp>
    typename std::enable_if_t< severity >= INFO, void >
    print_msg(const str_const& severity_desc, const std::string& message, Tp... args) {
        if (config->verbose)
            printf(message, args...);
    }

    template<unsigned int severity, int priority, typename... Tp>
    typename std::enable_if_t< severity <= INFO, void >
    call_syslog(const std::string& message, Tp... args) {
        if (config->syslog)
            syslog<priority>(message, args...);;
    }

    // We don't send debug messages to the system log
    template<unsigned int severity, int priority, typename... Tp>
    typename std::enable_if_t< severity >= DEBUG, int >
    call_syslog(const std::string& message, Tp... args) {
        return 0;
    }

    template<unsigned int severity, typename... Tp>
    int __emit_error(const std::string& message, Tp... args);

    template<unsigned int severity, typename... Tp>
    typename std::enable_if_t< severity <= INFO, int >
    emit_error(const std::string& message, Tp... args) {
        return __emit_error<severity>(message, args...);
    }

    template<unsigned int severity, typename... Tp>
    typename std::enable_if_t< severity >= DEBUG, int >
    emit_error(const std::string& message, Tp... args) {
        return 0;
    }
};

} // namespace kserver

#endif // __KSERVER_SYSLOG_HPP__
