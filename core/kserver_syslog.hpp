/// Server system log
///
/// (c) Koheron

#ifndef __KSERVER_SYSLOG_HPP__
#define __KSERVER_SYSLOG_HPP__

#include "config.hpp"

#include <memory>
#include <cstdarg>
#include <string>
#include <cstring>
#include <tuple>

#include <syslog.h>

namespace kserver {

class KServer;

#define FMT_BUFF_LEN 512

// ----------------------------------------------------------------------------------------------------
// str_const: TODO put in separate source file

// http://stackoverflow.com/questions/15858141/conveniently-declaring-compile-time-strings-in-c
class str_const { // constexpr string
  private:
    const char* const p_;
    const std::size_t sz_;

  public:
    template<std::size_t N>
    constexpr str_const(const char(&a)[N]) : // ctor
        p_(a), sz_(N-1) {}

    constexpr char operator[](std::size_t n) { // []
        return n < sz_ ? p_[n] :
        throw std::out_of_range("");
    }

    constexpr std::size_t size() const {return sz_;} // size()

    constexpr const char* data() const {return p_;}
};

// ----------------------------------------------------------------------------------------------------

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

        print_stdout<severity>(std::get<1>(log_array[severity]), std::string(message), args...);

        if (config->syslog)
            syslog(std::get<0>(log_array[severity]), message, args...);
    }

    template<unsigned int severity, typename... Tp>
    int print_stdout(const str_const& header, const std::string& message, Tp... args)
    {
        if (severity <= WARNING) {
            auto fmt = std::string(header.data()) + ": " + message;
            fprintf(stderr, fmt.data(), args...);
        } else { // INFO
            if (config->verbose)
                printf(message.data(), args...);
        }

        return 0;
    }

    template<unsigned int severity>
    int emit_error(const char *message, va_list argptr);
};

} // namespace kserver

#endif // __KSERVER_SYSLOG_HPP__
