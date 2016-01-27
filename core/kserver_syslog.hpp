/// Server system log
///
/// (c) Koheron

#ifndef __KSERVER_SYSLOG_HPP__
#define __KSERVER_SYSLOG_HPP__

#include "config.hpp"

#if KSERVER_HAS_THREADS
#  include <mutex>
#endif

namespace kserver {

#define FMT_BUFF_LEN 512

struct SysLog
{
    SysLog(KServerConfig *config_);
    ~SysLog();
    void close();

    /// Severity of the message
    enum severity {
        PANIC,    ///< When KServer is not functionnal anymore
        CRITICAL, ///< When an error results in session crash
        ERROR,    ///< Typically when a command execution failed
        WARNING,
        INFO,
        DEBUG,
        syslog_severity_num
    };
    
    void print(unsigned int severity, const char *message, ...);
    
private:
    KServerConfig *config;
    
    char fmt_buffer[FMT_BUFF_LEN];
    
    int print_stderr(const char *header, const char *message, ...);
    
#if KSERVER_HAS_THREADS
    std::mutex mutex;
#endif
};

} // namespace kserver

#endif // __KSERVER_SYSLOG_HPP__
