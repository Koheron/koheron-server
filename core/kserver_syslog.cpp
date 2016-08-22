/// Implementation of kserver_syslog.hpp
///
/// (c) Koheron

#include "kserver_syslog.hpp"

#include <stdio.h>
#include <syslog.h>
#include <cstring>

#if KSERVER_HAS_THREADS
#  include <thread>
#endif

#include "kserver_defs.hpp"
#include "kserver.hpp"

namespace kserver {

SysLog::SysLog(std::shared_ptr<KServerConfig> config_, KServer *kserver_)
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
void SysLog::close()
{
    assert(config != nullptr);

    if (config->syslog) {
        print<INFO>("Close syslog ...\n");
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

template<>
void SysLog::notify<SysLog::PANIC>(const char *message, va_list argptr)
{
    va_list argptr1, argptr2;
    va_copy(argptr1, argptr);
    va_copy(argptr2, argptr);

    print_stderr("KSERVER PANIC", message, argptr1);

    if (config->syslog)
        vsyslog(LOG_ALERT, message, argptr2);

    va_end(argptr1);
    va_end(argptr2);
}

template<>
void SysLog::notify<SysLog::CRITICAL>(const char *message, va_list argptr)
{
    va_list argptr1, argptr2;
    va_copy(argptr1, argptr);
    va_copy(argptr2, argptr);

    print_stderr("KSERVER CRITICAL", message, argptr1);

    if (config->syslog)
        vsyslog(LOG_CRIT, message, argptr2);

    va_end(argptr1);
    va_end(argptr2);
}

template<>
void SysLog::notify<SysLog::ERROR>(const char *message, va_list argptr)
{
    va_list argptr1, argptr2;
    va_copy(argptr1, argptr);
    va_copy(argptr2, argptr);

    print_stderr("KSERVER ERROR", message, argptr1);

    if (config->syslog)
        vsyslog(LOG_ERR, message, argptr2);

    va_end(argptr1);
    va_end(argptr2);
}

template<>
void SysLog::notify<SysLog::WARNING>(const char *message, va_list argptr)
{
    va_list argptr1, argptr2;
    va_copy(argptr1, argptr);
    va_copy(argptr2, argptr);

    print_stderr("KSERVER WARNING", message, argptr1);

    if (config->syslog)
        vsyslog(LOG_WARNING, message, argptr2);

    va_end(argptr1);
    va_end(argptr2);
}

template<>
void SysLog::notify<SysLog::INFO>(const char *message, va_list argptr)
{
    va_list argptr1;
    va_copy(argptr1, argptr);

    if (config->verbose)
        vprintf(message, argptr);

    if (config->syslog)
        vsyslog(LOG_NOTICE, message, argptr1);

    va_end(argptr1);
}

} // namespace kserver
