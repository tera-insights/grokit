/* Headef file containing macros for capturing profiling information

 */

#include <ctime>
#include <cinttypes>
#include "ProfMSG.h"
#include "Timer.h"
#include "Profiler.h"
#include "Logging.h"
#include "Pragma.h"

/*
 * General structure of code being profiled:
 *
 * <set up counters>
 * PROFILING2_START
 *
 *  <Important code to run>
 *
 * PROFILING2_END
 * <finalize counters>
 *
 * PROFILING2_SINGLE or PROFILING2_SET
 * depending on the number of counters
 */

// Saves the relevant start times for a profiling run
#define PROFILING2_START \
    timespec _dp_prof_wall_start_; \
    clock_gettime(CLOCK_REALTIME, &_dp_prof_wall_start_); \
    int64_t _dp_prof_wall_start_millis_ = (_dp_prof_wall_start_.tv_sec * 1000) + (_dp_prof_wall_start_.tv_nsec / 1000000);

// Saves the relevant end times for a profiling run
#define PROFILING2_END \
    timespec _dp_prof_wall_end_; \
    clock_gettime(CLOCK_REALTIME, &_dp_prof_wall_end_); \
    int64_t _dp_prof_wall_end_millis_ = (_dp_prof_wall_end_.tv_sec * 1000) + (_dp_prof_wall_end_.tv_nsec / 1000000);

// macro to increment value of counter(a string) by value(a long int)
#define PROFILING2_SINGLE(counter, value, group) { \
    PCounter cnt((counter), (value), (group)); \
    ProfileMessage_Factory(globalProfiler, _dp_prof_wall_start_millis_, \
            _dp_prof_wall_end_millis_, cnt); \
}

// macro to send a set of counters
#define PROFILING2_SET(counterSet, group) { \
    ProfileSetMessage_Factory(globalProfiler, _dp_prof_wall_start_millis_, \
            _dp_prof_wall_end_millis_, (counterSet)); \
}

// A counter that pertains to an instant in time instead of an interval
#define PROFILING2_INSTANT(counter, value, group) { \
    timespec _dp_prof_wall_instant_; \
    clock_gettime(CLOCK_REALTIME, &_dp_prof_wall_instant_); \
    int64_t _dp_prof_wall_instant_millis_ = (_dp_prof_wall_instant_.tv_sec * 1000) + (_dp_prof_wall_instant_.tv_nsec / 1000000); \
    PCounter cnt((counter), (value), (group)); \
    ProfileInstantMessage_Factory(globalProfiler, _dp_prof_wall_instant_millis_, cnt); \
}

#define PROFILING2_PROGRESS_SINGLE(counter, value, group) { \
    timespec _dp_prof_wall_instant_; \
    clock_gettime(CLOCK_REALTIME, &_dp_prof_wall_instant_); \
    int64_t _dp_prof_wall_instant_millis_ = (_dp_prof_wall_instant_.tv_sec * 1000) + (_dp_prof_wall_instant_.tv_nsec / 1000000); \
    PCounter cnt((counter), (value), (group)); \
    ProfileProgressMessage_Factory(globalProfiler, _dp_prof_wall_instant_millis_, cnt); \
}

#define PROFILING2_PROGRESS_SET(counterSet, group) { \
    timespec _dp_prof_wall_instant_; \
    clock_gettime(CLOCK_REALTIME, &_dp_prof_wall_instant_); \
    int64_t _dp_prof_wall_instant_millis_ = (_dp_prof_wall_instant_.tv_sec * 1000) + (_dp_prof_wall_instant_.tv_nsec / 1000000); \
    ProfileProgressSetMessage_Factory(globalProfiler, _dp_prof_wall_instant_millis_, (counterSet)); \
}

#define PROFILING2(counter, value) \
    PRAGMA_MSG("The PROFILING2 macro has been deprecated.")

// macro to flush profiling info to screen if the second is up
#define PROFILING2_FLUSH \
    PRAGMA_MSG("The PROFILING2_FLUSH macro has been deprecated.")
