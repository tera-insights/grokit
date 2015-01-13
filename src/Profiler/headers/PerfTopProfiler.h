// Copyright 2013 Tera Insights, LLC. All Rights Reserved.
// Author: Christopher Dudley

#ifndef _PERF_TOP_PROFILER_H_
#define _PERF_TOP_PROFILER_H_

#include "EventGenerator.h"
#include "EventProcessor.h"

#include <cstdio>

class PerfTopProfilerImp;

class PerfTopProfiler : public EventGenerator {
    public:
        PerfTopProfiler(void): EventGenerator() { }

        PerfTopProfiler(EventProcessor& _profiler);
};

class PerfTopProfilerImp : public EventGeneratorImp {
private:
    static constexpr size_t BUFFER_SIZE = 65536; // 64 K should be enough for anyone

    EventProcessor myProfiler; // the profiler we send messages to

    pid_t childID;
    int childIn;
    int childOut;
    int childErr;

    char buffer[BUFFER_SIZE];      // Main text buffer
    char * start;       // Start of unread text in buffer should be enough for anyone should be enough for anyone
    char * end;         // One past the end of unread text in buffer

public:
    PerfTopProfilerImp(EventProcessor& _profiler);

    virtual void PreStart() override;

    virtual int ProduceMessage() override;

    ~PerfTopProfilerImp() { }
};

#endif // _PERF_TOP_PROFILER_H_
