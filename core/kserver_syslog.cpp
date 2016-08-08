/// Implementation of kserver_syslog.hpp
///
/// (c) Koheron

#include "kserver_syslog.hpp"

#include <stdio.h>
#include <syslog.h>
#include <cstdarg>
#include <cstring>

#if KSERVER_HAS_THREADS
#  include <thread>
#endif

#include "kserver_defs.hpp"

namespace kserver {

SysLog::SysLog(std::shared_ptr<KServerConfig> config_)
: config(config_)
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
void SysLog::close()
{
    assert(config != nullptr);

    if (config->syslog) {
        print(INFO, "Close syslog ...\n");
        closelog();
    }
}

int SysLog::print_stderr(const char *header, const char *message, va_list argptr)
{
    int ret = snprintf(fmt_buffer, FMT_BUFF_LEN, "%s: %s", header, message);

    if (ret < 0) {
        fprintf(stderr, "Format error\n");
        return -1;
    }

    if (ret >= FMT_BUFF_LEN) {
        fprintf(stderr, "Buffer fmt_buffer overflow\n");
        return -1;
    }

    vfprintf(stderr, fmt_buffer, argptr);
    return 0;
}

void SysLog::print(unsigned int severity, const char *message, ...)
{    
#if KSERVER_HAS_THREADS
    std::lock_guard<std::mutex> lock(mutex);
#endif

    va_list argptr, argptr2, argptr3;
    va_start(argptr, message);

    // See http://comments.gmane.org/gmane.linux.suse.programming-e/1107
    va_copy(argptr2, argptr);
    va_copy(argptr3, argptr);

    switch (severity) {
      case PANIC:
        print_stderr("KSERVER PANIC", message, argptr3);

        if (config->syslog)
            vsyslog(LOG_ALERT, message, argptr2);

        break;
      case CRITICAL:
        print_stderr("KSERVER CRITICAL", message, argptr3);

        if (config->syslog)
            vsyslog(LOG_CRIT, message, argptr2);

        break;
      case ERROR:
        print_stderr("KSERVER ERROR", message, argptr3);

        if (config->syslog)
            vsyslog(LOG_ERR, message, argptr2);

        break;
      case WARNING:
        print_stderr("KSERVER WARNING", message, argptr3);

        if (config->syslog)
            vsyslog(LOG_WARNING, message, argptr2);

        break;
      case INFO:
        if (config->verbose)
            vprintf(message, argptr);

        if (config->syslog)
            vsyslog(LOG_NOTICE, message, argptr2);

        break;
      case DEBUG:
        if (config->verbose)
            vprintf(message, argptr);

        if (config->syslog)
            vsyslog(LOG_DEBUG, message, argptr2);

        break;
      default:
        fprintf(stderr, "Invalid severity level\n");
    }

    va_end(argptr3);
    va_end(argptr2);
    va_end(argptr);
}

} // namespace kserver
