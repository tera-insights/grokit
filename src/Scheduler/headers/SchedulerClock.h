// Copyright 2013 Tera Insights, LLC. All Rights Reserved.
// Author: Christopher Dudley

#ifndef _SCHEDULER_CLOCK_H_
#define _SCHEDULER_CLOCK_H_

#include <ctime>

class SchedulerClockImp : public EventGeneratorImp {
    EventProcessor recipient;

    // The period of the clock
    const timespec period;

    // The current time
    timespec current;

    static constexpr unsigned long long NS_PER_SEC = 1000000000;

    public:
        // Creates a new clock with a period _ns nanoseconds long
        SchedulerClockImp( unsigned long long _ns, 
			   EventProcessor &recipient );

        virtual void PreStart(void) override;
        virtual int ProduceMessage(void) override;
};

class SchedulerClock : public EventGenerator {

    public:
      SchedulerClock( unsigned long long _ns, 
		      EventProcessor &recipient );
};

#endif//_SCHEDULER_CLOCK_H_
