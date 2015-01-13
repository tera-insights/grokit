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
#ifndef _PC_PROFILER_H_
#define _PC_PROFILER_H_

/** Class that measures performance of the system/second

    Result is logged in a file

*/

#include "EventGenerator.h"
#include "EventProcessor.h"

#include <iostream>
#include <cinttypes>
#include <ctime>

#include "Logging.h"
#include "PerfCounter.h"
#include "json.h"

class PCProfilerImp; // forward definition

class PCProfiler : public EventGenerator{
    public:
        PCProfiler(void):EventGenerator(){}

        PCProfiler(EventProcessor& _profiler);
};

class PCProfilerImp : public EventGeneratorImp {
    private:

        // The target time of our sleep
        timespec targetTime;

        EventProcessor myProfiler; // the profiler we send messages to

        Json::Value statInfo;

        // Function used to transform cpu stat values from (1/USER_HZ) to milliseconds
        // We do this here so we pay the cost of figuring out the ratio exactly
        // once, avoiding branches in the future.
        std::function<int64_t(int64_t)> cpuStatToMillis;

        // Get information from /proc/stat, put the new info into statInfo
        // and the differences into diff
        void GetStatInfo( Json::Value & diff );

    public:
        PCProfilerImp(EventProcessor& profiler);

        virtual int ProduceMessage() override;

        virtual void PreStart() override;

        ~PCProfilerImp(){ }
};

#endif // _PC_PROFILER_H_
