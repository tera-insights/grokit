// Copyright 2013 Tera Insights, LLC. All Rights Reserved.
// Author: Christopher Dudley

#include "SchedulerClock.h"
#include <ctime>

#include "SchedulerMessages.h"

// Static constants

constexpr unsigned long long SchedulerClockImp :: NS_PER_SEC;

// Members

SchedulerClockImp :: SchedulerClockImp( unsigned long long _ns ) :
    period { _ns / NS_PER_SEC, _ns % NS_PER_SEC }
{ }

void SchedulerClockImp :: PreStart(void) {
    clock_gettime(CLOCK_MONOTONIC, &current);
}

int SchedulerClockImp :: ProduceMessage(void) {
    // Calculate the next absolute time
    current.tv_sec += (current.tv_nsec + period.tv_nsec) / NS_PER_SEC;
    current.tv_sec += period.tv_sec;
    current.tv_nsec = (current.tv_nsec + period.tv_nsec) % NS_PER_SEC;

    // Wait until the time
    while( clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &current, NULL ) != 0 );
}
