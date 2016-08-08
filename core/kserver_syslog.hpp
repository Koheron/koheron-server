/// Server system log
///
/// (c) Koheron

#ifndef __KSERVER_SYSLOG_HPP__
#define __KSERVER_SYSLOG_HPP__

#include "config.hpp"

#include <memory>
#include <cstdarg>

namespace kserver {

#define FMT_BUFF_LEN 512

struct SysLog
{
    SysLog(std::shared_ptr<KServerConfig> config_);

    void close();

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

    void print(unsigned int severity, const char *message, ...);

    void print_dbg(const char *message, ...)
    {
        if (config->verbose) {
            va_list argptr;
            va_start(argptr, message);
            vprintf(message, argptr);
        }
    }

private:
    std::shared_ptr<KServerConfig> config;
    char fmt_buffer[FMT_BUFF_LEN];

    int print_stderr(const char *header, const char *message, va_list argptr);
};

} // namespace kserver

#endif // __KSERVER_SYSLOG_HPP__
