//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#include <array>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <string>
#include <functional>

#include <ctime>
#include <cstdio>
#include <cinttypes>

#include <boost/algorithm/string/predicate.hpp>

#include <unistd.h>

#include "PCProfiler.h"
#include "PerfCounter.h"
#include "Profiler.h"
#include "Timer.h"
#include "ProfMSG.h"

using std::string;
using std::ifstream;
using std::istringstream;
using boost::algorithm::starts_with;

// The fields of the "cpu" line in /proc/stat
static constexpr int N_CPU_FIELDS = 10;
static const std::array<std::string, N_CPU_FIELDS> procStatCpuFields = { "CPU User", "CPU Nice", "CPU System", "CPU Idle",
    "CPU Iowait", "CPU Irq", "CPU Softirq", "CPU Steal", "CPU Guest", "CPU Guest_nice" };
static const std::unordered_set<std::string> procStatCpuFieldsUsed =
    { "CPU User", "CPU Nice", "CPU System", "CPU Irq", "CPU Softirq", "CPU Steal", "CPU Guest", "CPU Guest_nice" };

PCProfiler :: PCProfiler(EventProcessor& profiler){
  evGen = new PCProfilerImp(profiler);
}

PCProfilerImp::PCProfilerImp(EventProcessor& profiler) :
    targetTime()
{
    myProfiler.copy( profiler );

    // Get the USER_HZ value that many values in /proc/stat are measured by.
    // It is normally 100, so the values are normally measured in 1/100ths of a second.
    const long user_hz = sysconf(_SC_CLK_TCK);

    if( user_hz < 1000 ) {
        const int64_t ratio = 1000 / user_hz;
        cpuStatToMillis = [ratio] (int64_t val) { return val * ratio; };
    }
    else if (user_hz == 1000) {
        // just the identity function
        cpuStatToMillis = [] (int64_t val) { return val; };
    }
    else {
        const int64_t ratio = user_hz / 1000;
        cpuStatToMillis = [ratio] (int64_t val) { return val / ratio; };
    }
}

void PCProfilerImp :: GetStatInfo( Json::Value & diff ) {
    ifstream proc("/proc/stat");

    Json::Value nStats;
    string line;

    // TODO: Change to "while" if we do multiple lines
    if ( getline(proc, line) ) {
        istringstream ss(line);
        string rawType; // the type of line
        ss >> rawType;

        // Prefix with an asterisk so that it is easy to tell that this is
        // non-waypoint information
        string type = "_" + rawType;

        if( rawType == "cpu" ) {
            // CPU line
            for( auto field : procStatCpuFields ) {
                int64_t val;
                ss >> val;
                if( procStatCpuFieldsUsed.find(field) != procStatCpuFieldsUsed.end() ) {
                    val = cpuStatToMillis(val);
                    nStats[field][type] = (Json::Int64) val;
                    diff[field][type] = (Json::Int64) val - statInfo[field][type].asInt64();
                }
            }
        }
        // Add other types in the future if we want them
    }

    nStats.swap(statInfo);
}

void PCProfilerImp :: PreStart(void) {
    clock_gettime(CLOCK_MONOTONIC, &targetTime);
    Json::Value dummy;
    GetStatInfo(dummy);
}

int PCProfilerImp::ProduceMessage(){
    targetTime.tv_sec += 1;

    while( clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &targetTime, NULL ) != 0 ) {
        // nothing
    }

    timespec wallTime;
    timespec cpuTime;

    clock_gettime(CLOCK_REALTIME, &wallTime);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cpuTime);

    int64_t wall = (wallTime.tv_sec * 1000) + (wallTime.tv_nsec / 1000000);
    int64_t cpu = (cpuTime.tv_sec * 1000) + (cpuTime.tv_nsec / 1000000);

    Json::Value diff;
    GetStatInfo(diff);

    ProfileIntervalMessage_Factory(myProfiler, wall, cpu, diff);

    return 0;
}

