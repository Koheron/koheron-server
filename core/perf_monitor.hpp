/// @file perf_monitor.hpp
///
/// @brief Session performance monitor
///
/// @author Thomas Vanderbruggen <thomas@koheron.com>
/// @date 19/09/2015
///
/// (c) Koheron 2014-2015

#ifndef __PERF_MONITOR_HPP__
#define __PERF_MONITOR_HPP__

#include <chrono>
#include <array>
#include <cassert>

namespace kserver {

typedef long long counts_t;

/// List of the points to be measured
typedef enum timing_points {
    READY_TO_READ,
    PARSE,
    EXECUTE,
    timing_points_num
} timing_point_t;

/// Timing points descriptions
const std::array< std::string, timing_points_num > 
timing_points_desc = {{
	"Ready to read",
	"Parse",
	"Execute"
}};

// Timing points:
// t(0) --> ... --> t(i-1) --> t(i) --> ... --> t(N-1)
//
// Durations:
// Dt(i) = t(i) - t(i-1) 

class PerfMonitor
{
  public:
    PerfMonitor();
    
    void tic(timing_point_t time_pt);
    
    /// Return the mean duration of a given step
    float get_mean_duration(timing_point_t time_pt) const;
    
    /// Return the minimum duration of a given step
    inline int get_min_duration(timing_point_t time_pt) const
    {
        assert(time_pt < timing_points_num);
        return min_duration;
    }
    
    /// Return the maximum duration of a given step
    inline int get_max_duration(timing_point_t time_pt) const
    {
        assert(time_pt < timing_points_num);
        return max_duration;
    }
    
  private:
    /// Number of times we closed the session loop,
    /// i.e. when we came back to READY_TO_READ
    int num_sess_loop;
  
    /// Previous time
    std::chrono::steady_clock::time_point prev_time;
    
    /// Durations inbetween timing points
    std::array<counts_t, timing_points_num> durations;
    
    /// Minimum duration
    int min_duration;
    
    /// Maximum duration
    int max_duration;
    
}; // PerfMonitor

} // namespace kserver

#endif // __PERF_MONITOR_HPP__

