// Copyright 2013 Tera Insights, LLC. All Rights Reserved.
// Author: Christopher Dudley

#ifndef _SCHEDULER_EVENT_SINGLE_H_
#define _SCHEDULER_EVENT_SINGLE_H_

#include "EventProcessor.h"
#include "Data.h"

#include <cinttypes>
#include <utility>

/**
 *  This class represents an event scheduled to run exactly once.
 *
 *  The event consists of an expiry time for for the event, an optional ID,
 *  a payload to be sent when the event expires, and the destination to which
 *  to send the payload.
 */
class SchedulerEventSingle {
    // A unique ID for event.
    int64_t event_id;

    // The time this event expires (in ticks)
    int64_t expiry_time;

    // The destination of the message to be sent when this event expires
    EventProcessor destination;

    // The data payload to be sent along with the message
    Data payload;

public:

    // Default construct an empty, expired event
    SchedulerEventSingle(void) noexcept :
        event_id(-1),
        expiry_time(-1),
        destination(),
        payload()
    { }

    // Construction from components
    SchedulerEventSingle( const int64_t id, const int64_t et, EventProcessor& dest, Data load& ) :
        event_id(id),
        expiry_time(et)
    {
        destination.swap(dest);
        payload.swap(load);
    }

    // Moving is allowed
    SchedulerEventSingle( SchedulerEventSingle && o ) noexcept :
        event_id(o.event_id),
        expiry_time(o.expiry_time),
        destination(std::move(o.destination)),
        payload(std::move(o.payload))
    {
        // Invalidate other event
        o.event_id = -1;
        o.expiry_time = -1;
    }

    // No copying
    SchedulerEventSingle( const SchedulerEventSingle & ) = delete;
    SchedulerEventSingle & operator =( const SchedulerEventSingle & ) = delete;

    int64_t event_id(void) const noexcept {
        return event_id;
    }

    int64_t expiry_time(void) const noexcept {
        return expiry_time;
    }

    void extract_destination(EventProcessor& where) {
        where.swap(destination);
    }

    void extract_payload(Data& where) {
        where.swap(payload);
    }

    // Set up operators, so we can use this with a priority queue
    bool operator < (const SchedulerEventSingle & o) const noexcept {
        return expiry_time < o.expiry_time;
    }

    bool operator > (const SchedulerEventSingle & o) const noexcept {
        return expiry_time > o.expiry_time;
    }

    bool operator <= (const SchedulerEventSingle & o) const noexcept {
        return expiry_time <= o.expiry_time;
    }

    bool operator >= (const SchedulerEventSingle & o) const noexcept {
        return expiry_time > o.expiry_time;
    }
};

#endif//_SCHEDULER_EVENT_SINGLE_H_
