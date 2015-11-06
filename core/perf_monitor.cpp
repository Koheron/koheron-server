/// @file perf_monitor.cpp
///
/// @brief Implementation of perf_monitor.hpp
///
/// @author Thomas Vanderbruggen <thomas@koheron.com>
/// @date 19/09/2015
///
/// (c) Koheron 2014-2015

#include "perf_monitor.hpp"

namespace kserver {

PerfMonitor::PerfMonitor()
{
    num_sess_loop = -1;
        
    for(int time_pt=0; time_pt<timing_points_num; time_pt++)
        durations[time_pt] = 0;
        
    min_duration = -1;
    max_duration = -1;
}

void PerfMonitor::tic(timing_point_t time_pt)
{    
    assert(time_pt < timing_points_num);
    assert((num_sess_loop == -1 && time_pt == READY_TO_READ) 
           || (num_sess_loop >= 0));
    
    auto now = std::chrono::steady_clock::now();
        
    if(num_sess_loop != -1) {
        int duration = std::chrono::duration_cast<std::chrono::microseconds>
                                                (now - prev_time).count();
        
        // We sum up the durations to provide
        // an average duration time dividing
        // by num_sess_loop afterwards.
        durations[time_pt] += duration;
        
        if(min_duration < 0 || duration < min_duration) 
            min_duration = duration;
            
        if(duration > max_duration) 
            max_duration = duration;
    }
                    
    prev_time = now;

    if(time_pt == READY_TO_READ)
        num_sess_loop++;
}

float PerfMonitor::get_mean_duration(timing_point_t time_pt) const
{
    assert(time_pt < timing_points_num);
    return static_cast<float>(durations[time_pt]) / (num_sess_loop + 1);
}

} // namespace kserver
