/// Generate devices definitions from device table
///
/// (c) Koheron

#ifndef __DEV_DEFINITIONS_HPP__
#define __DEV_DEFINITIONS_HPP__

#include <vector>
#include <devices_table.hpp>

namespace kserver {

/// Return the device description from device number
#define GET_DEVICE_NAME(dev_num) device_desc[dev_num][0]

/// Return the device description string of a Command cmd
#define GET_DEVICE_DESC(cmd) device_desc[cmd.device][0]

/// Return the operation description string of a Command cmd
#define GET_OPERATION_DESC(cmd) device_desc[cmd.device][cmd.operation+1]

/// Operation number
typedef unsigned int operation_t;

// String conversions
// Useful to convert op_param_array elements
#define STRING_TO_BOOL(str)	  ( (str).at(0) == '1' )
#define STRING_TO_ULONG(str)  strtoul((str).c_str(), NULL, 10)
#define STRING_TO_UINT(str)   (unsigned int) STRING_TO_ULONG(str)
#define STRING_TO_FLOAT(str)  (float) atof((str).c_str())

#define CSTRING_TO_BOOL(str)  ( str[0] == '1' )
#define CSTRING_TO_ULONG(str) strtoul(str, NULL, 10)
#define CSTRING_TO_UINT(str)  (unsigned int) CSTRING_TO_ULONG(str)
#define CSTRING_TO_FLOAT(str) (float) atof(str)

/// Execution status
typedef enum {
	exec_done,	    ///< Execution performed with success
	exec_err,	    ///< Error at execution
	exec_skip,	    ///< Execution skipped (because of earlier error)
	exec_pending,	///< Execution pending
	exec_status_num
} exec_status_t;

/// Status descriptions
const std::array< std::string, exec_status_num > 
exec_status_desc = {{
	"Done",
	"Error",
	"Skipped",
	"Pending"
}};

/// Return the status description string of a Command cmd
#define GET_EXEC_STATUS_DESC(cmd) exec_status_desc[cmd.status]

} // namespace kserver

#endif //__DEV_DEFINITIONS_HPP__
