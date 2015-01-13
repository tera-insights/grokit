#include "PerfCounter.h"

const char* PerfCounter::names[] = {
    "cyl",                  // Cycles
    "ins",                  // Instructions
    "car",                  // Cache References
    "cam",                  // Cache Misses
    "bri",                  // Branch Instructions
    "brm",                  // Branch Misses
    "cts",                  // Context Switches
    "tck"                   // Task Clock
};
