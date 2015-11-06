/// @file commands.hpp
///
/// @brief Commands for KServer
///
/// @author Thomas Vanderbruggen <thomas@koheron.com>
/// @date 22/11/2014
///
/// (c) Koheron 2014

#ifndef __COMMANDS_HPP__
#define __COMMANDS_HPP__

#include "session_manager.hpp"
#include "dev_definitions.hpp"

#if USE_BOOST
  #include <boost/circular_buffer.hpp>
#else
  #include <vector>
#endif

namespace kserver {

/// @brief Command parameters
struct Command
{
    SessID sess_id = -1;            ///< ID of the session emitting the command  
    device_t device = NO_DEVICE;    ///< The device to control
    uint32_t operation = -1;        ///< Operation ID
    char* buffer = nullptr;         ///< data buffer

    bool parsing_err = 0;           ///< True if parsing error
    exec_status_t status = exec_pending; ///< Execution status
    
    /// @brief Print the content of the command
    void print(void);
};

#if USE_BOOST
  typedef boost::circular_buffer<Command> cmd_log_container;
#else
  typedef std::vector<Command> cmd_log_container;
#endif

/// Log of the commands 
struct CommandLog : public cmd_log_container
{
    CommandLog(unsigned int depth_log_)
    : cmd_log_container(depth_log_),
      requests_num(0), errors_num(0)
    {}

    unsigned int requests_num; ///< Number of requests received
    unsigned int errors_num;   ///< Number of requests that resulted in an error
};

} // namespace kserver

#endif // __COMMANDS_HPP__

