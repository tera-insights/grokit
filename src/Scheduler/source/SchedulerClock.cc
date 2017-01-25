// Copyright 2013 Tera Insights, LLC. All Rights Reserved.
// Author: Christopher Dudley

#include <ctime>

#include "EEMessageTypes.h"
#include "SchedulerClock.h"
#include "SchedulerMessages.h"

// Static constants

constexpr unsigned long long SchedulerClockImp :: NS_PER_SEC;

// Members

SchedulerClock :: SchedulerClock ( unsigned long long _ns,
				   EventProcessor &r ) {
  evProc = new SchedulerClockImp(_ns, r);
}

SchedulerClockImp :: SchedulerClockImp( unsigned long long _ns,
					EventProcessor &r ) :
  period { _ns / NS_PER_SEC, _ns % NS_PER_SEC } {
    recipient.copy(r);
}

void SchedulerClockImp :: PreStart(void) {
    clock_gettime(CLOCK_MONOTONIC, &current);
}

int SchedulerClockImp :: ProduceMessage(void) {
    // Calculate the next absolute time
    current.tv_sec += (current.tv_nsec + period.tv_nsec) / NS_PER_SEC;
    current.tv_sec += period.tv_sec;
    current.tv_nsec = (current.tv_nsec + period.tv_nsec) % NS_PER_SEC;

    while( clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &current, 
			    NULL ) != 0 ) {}

    TickMessage_Factory(recipient);
    return 0;
}
