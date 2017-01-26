// Copyright 2013 Tera Insights, LLC. All Rights Reserved.
// Author: Christopher Dudley

#include <ctime>

#include "EEExternMessages.h"
#include "SchedulerClock.h"

// Static constants

constexpr unsigned long long SchedulerClockImp :: NS_PER_SEC;

// Members

SchedulerClock :: SchedulerClock ( unsigned long long _ns,
				   EventProcessor &r ) {
  this->evGen = new SchedulerClockImp(_ns, r);
}

SchedulerClockImp :: SchedulerClockImp( unsigned long long _ns,
					EventProcessor &r ) :
  period { static_cast<time_t>(_ns / NS_PER_SEC), 
    static_cast<long>(_ns % NS_PER_SEC)} {
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
