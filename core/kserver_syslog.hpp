/// Server system log
///
/// (c) Koheron

#ifndef __KSERVER_SYSLOG_HPP__
#define __KSERVER_SYSLOG_HPP__

#include "config.hpp"

#include <memory>
#include <cstdarg>

namespace kserver {

class KServer;

#define FMT_BUFF_LEN 512

struct SysLog
{
    SysLog(std::shared_ptr<KServerConfig> config_, KServer *kserver_);

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

    template<unsigned int severity>
    void print(const char *message, ...);

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
    KServer *kserver;

    template<unsigned int severity>
    void notify(const char *message, va_list argptr);

    int print_stderr(const char *header, const char *message, va_list argptr);

    template<unsigned int severity>
    int emit_error(const char *message, va_list argptr);
};

} // namespace kserver

#endif // __KSERVER_SYSLOG_HPP__
