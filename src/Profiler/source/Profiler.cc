// Copyright 2013 Tera Insights. LLC. All Rights Reserved.

#include "Profiler.h"
#include "PerfProfMSG.h"
#include "Timer.h"
#include "Logging.h"
#include "Stl.h"
#include "CommunicationFramework.h"
#include "SerializeJson.h"
#include <inttypes.h>

#ifdef MMAP_DIAG_TICK
#include "MmapAllocator.h"
#endif



#include <sqlite3.h>
#include <assert.h>
#include "Debug.h"
#include "Errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

static double millisToDouble( int64_t millis ) {
    return millis / 1000.0;
}

static int64_t timespecToMillis( const timespec & t ) {
    return (t.tv_sec * 1000) + (t.tv_nsec / 1000000);
}

static double timespecToDouble( const timespec & t ) {
    return t.tv_sec + (t.tv_nsec * 1e-9);
}

// Instantiate global profiler
Profiler globalProfiler;

ProfilerImp::ProfilerImp( bool suppress ): lastCpu(0), lastTick(0)
    , suppressOutput(suppress)
#ifdef  DEBUG_EVPROC
    ,EventProcessorImp(true, "Profiler")
#endif
{
    RegisterMessageProcessor(ProfileMessage::type, &ProfileMessage_H, 2);
    RegisterMessageProcessor(ProfileSetMessage::type, &ProfileSetMessage_H, 2);
    RegisterMessageProcessor(ProfileIntervalMessage::type, &ProfileIntervalMessage_H, 2);
    RegisterMessageProcessor(ProfileInstantMessage::type, &ProfileInstantMessage_H, 2);
    RegisterMessageProcessor(ProfileProgressMessage::type, &ProfileProgressMessage_H, 2);
    RegisterMessageProcessor(ProfileProgressSetMessage::type, &ProfileProgressSetMessage_H, 2);
    RegisterMessageProcessor(PerfTopMessage::type, &PerfTopMessage_H, 2);
}

void ProfilerImp::PreStart(void) {
    HostAddress frontend;
    GetFrontendAddress(frontend);
    MailboxAddress loggerAddr(frontend, "logger");
    FindRemoteEventProcessor(loggerAddr, logger);

    timespec lastWallSpec;
    timespec lastCpuSpec;
    clock_gettime(CLOCK_REALTIME, &lastWallSpec);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &lastCpuSpec);

    beginTime = timespecToDouble(lastWallSpec);
    lastCpu = timespecToMillis(lastWallSpec);
    lastTick = timespecToMillis(lastWallSpec);
}

void ProfilerImp::SuppressOutput(bool v) {
    suppressOutput = v;
}

ProfilerImp :: ~ProfilerImp() {
}

MESSAGE_HANDLER_DEFINITION_BEGIN(ProfilerImp, ProfileMessage_H, ProfileMessage){

    evProc.AddCounter(msg.counter);

#ifdef PROFILER_SEND_EVENTS
    ProfileMessage::Factory(evProc.logger, msg);
#endif //PROFILER_SEND_EVENTS

}MESSAGE_HANDLER_DEFINITION_END


MESSAGE_HANDLER_DEFINITION_BEGIN(ProfilerImp, ProfileSetMessage_H, ProfileSetMessage){

    FOREACH_TWL(el, msg.counters){
        evProc.AddCounter(el);
    }END_FOREACH

#ifdef PROFILER_SEND_EVENTS
    ProfileSetMessage::Factory(evProc.logger, msg);
#endif //PROFILER_SEND_EVENTS

}MESSAGE_HANDLER_DEFINITION_END

MESSAGE_HANDLER_DEFINITION_BEGIN(ProfilerImp, ProfileInstantMessage_H, ProfileInstantMessage) {
    evProc.AddCounter(msg.counter);

#ifdef PROFILER_SEND_EVENTS
    ProfileInstantMessage::Factory(evProc.logger, msg);
#endif //PROFILER_SEND_EVENTS
}MESSAGE_HANDLER_DEFINITION_END

MESSAGE_HANDLER_DEFINITION_BEGIN(ProfilerImp, ProfileProgressMessage_H, ProfileProgressMessage) {
    evProc.AddCounterProg(msg.counter);

#ifdef PROFILER_SEND_EVENTS
    ProfileProgressMessage::Factory(evProc.logger, msg);
#endif //PROFILER_SEND_EVENTS
}MESSAGE_HANDLER_DEFINITION_END

MESSAGE_HANDLER_DEFINITION_BEGIN(ProfilerImp, ProfileProgressSetMessage_H, ProfileProgressSetMessage){

    FOREACH_TWL(el, msg.counters){
        evProc.AddCounterProg(el);
    }END_FOREACH

#ifdef PROFILER_SEND_EVENTS
    ProfileProgressSetMessage::Factory(evProc.logger, msg);
#endif //PROFILER_SEND_EVENTS
}MESSAGE_HANDLER_DEFINITION_END

MESSAGE_HANDLER_DEFINITION_BEGIN(ProfilerImp, ProfileIntervalMessage_H, ProfileIntervalMessage) {
    if( ! evProc.suppressOutput )
        evProc.PrintCounters(msg.cTime, msg.wallTime);

    Json::Value stats;
    stats.swap(msg.statInfo);

    // Add our aggregated information to the JSON and then send it
    for( auto & group : evProc.cMap ) {
        for ( auto & counter : group.second ) {
            stats[group.first][counter.first] = (Json::Int64) counter.second;
        }
    }

    evProc.cMap.clear();

    Json::Value progress(Json::objectValue);

    for( auto & group : evProc.progMap ) {
        for( auto & counter : group.second ) {
            progress[group.first][counter.first] = (Json::Int64) counter.second;
        }
    }

    evProc.progMap.clear();

    ProfileAggregateMessage::Factory(evProc.logger, evProc.lastTick, msg.wallTime, stats, progress );

    evProc.lastCpu = msg.cTime;
    evProc.lastTick = msg.wallTime;

#ifdef MMAP_DIAG_TICK
    MMAP_DIAG;
#endif
} MESSAGE_HANDLER_DEFINITION_END

MESSAGE_HANDLER_DEFINITION_BEGIN(ProfilerImp, PerfTopMessage_H, PerfTopMessage) {
    // Forward to logger
    PerfTopMessage::Factory( evProc.logger, msg.wallTime, msg.info );

} MESSAGE_HANDLER_DEFINITION_END

void ProfilerImp::PrintCounters(const int64_t newCpuSpec, const int64_t newClockSpec){
    std::ostringstream out;

    double oldCpu = millisToDouble(lastCpu);
    double newCpu = millisToDouble(newCpuSpec);

    double oldClock = millisToDouble(lastTick);
    double newClock = millisToDouble(newClockSpec);

    out << fixed << setprecision(2);
    out << "SC(" << setw(8) << (newClock - beginTime) << ")";
    out << "\tCPU:" << setw(5) << (newCpu-oldCpu)/(newClock-oldClock);
    FOREACH_STL(group, cMap){
        std::ostringstream gOut;
        gOut << endl << "[" << group.first << "]";
        if( group.first.length() < groupColWidth - 2 ) {
            gOut << std::string(groupColWidth - 2 - group.first.length(), ' ');
        }
        bool gotOne = false;
        FOREACH_STL(el, group.second) {
            std::string counter=el.first;
            int64_t& value=el.second;
            int64_t val = value;
            if (val>0){
                gotOne = true;
                gOut <<"\t";
                gOut << counter << ":";
                std::string outVal;
                HumanizeNumber( val, outVal );
                gOut << outVal;
            }
        }END_FOREACH;
        if( gotOne ) {
            out << gOut.str();
        }
    }END_FOREACH;

    cout << out.str() << endl;
}
