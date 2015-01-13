// Copyright 2013 Tera Insights, LLC. All Rights Reserved.
// Author: Christopher Dudley

#ifndef _TIME_POINT_H_
#define _TIME_POINT_H_

#include <cinttypes>

class TimePoint {

public:
    // Type used to represent the time.
    typedef uint64_t time_rep;

    // Static constants mostly to give names to the various constants used.
    static constexpr time_rep NS_PER_SEC = 1000000000;
    static constexpr time_rep US_PER_SEC = 1000000;
    static constexpr time_rep MS_PER_SEC = 1000;

    static constexpr time_rep NS_PER_MS  = 1000000;
    static constexpr time_rep NS_PER_US  = 1000;

    static constexpr time_rep SEC_PER_MIN = 60;
    static constexpr time_rep SEC_PER_HOUR = 3600;
    static constexpr time_rep SEC_PER_DAY = 86400;

    static constexpr time_rep MAX_NS = -1;

private:
    // The point in time / duration specified
    time_rep tval;

public:

    // Default constructor
    constexpr TimePoint( void ) noexcept :
        tval(0)
    {
    }

    // Copy constructor
    constexpr TimePoint( const TimePoint & o ) noexcept :
        tval(o.tval)
    {
    }
    // Copy assignment
    TimePoint & operator = (const TimePoint & o ) noexcept {
        tval = o.tval;
        return *this;
    }

    // Constructs a time point from a duration in nanoseconds
    constexpr TimePoint( const unsigned long long _ns ) noexcept :
        tval( _ns )
    {
    }

    // Accessor methods
    constexpr uint64_t nanos(void) const noexcept {
        return tval;
    }

    constexpr uint64_t micros(void) const noexcept {
        return tval / NS_PER_US;
    }

    constexpr uint64_t millis(void) const noexcept {
        return tval / NS_PER_MS;
    }

    constexpr uint64_t seconds(void) const noexcept {
        return tval / NS_PER_SEC;
    }

    constexpr uint64_t minutes(void) const noexcept {
        return tval / (SEC_PER_MIN * NS_PER_SEC);
    }

    constexpr uint64_t hours(void) const noexcept {
        return tval / (SEC_PER_HOUR * NS_PER_SEC);
    }

    constexpr uint64_t days(void) const noexcept {
        return tval / (SEC_PER_DAY * NS_PER_SEC);
    }

    // Comparison operators with other TimePoints
    constexpr bool operator < (const TimePoint & o) const {
        return tval < o.tval;
    }

    constexpr bool operator > (const TimePoint & o) const {
        return tval > o.tval;
    }

    constexpr bool operator <= (const TimePoint & o) const {
        return tval <= o.tval;
    }

    constexpr bool operator >= (const TimePoint & o) const {
        return tval >= o.tval;
    }

    constexpr bool operator == (const TimePoint & o) const {
        return tval == o.tval;
    }

    constexpr bool operator != (const TimePoint & o) const {
        return tval != o.tval;
    }

    // Arithmetic operators with other TimePoints
    constexpr TimePoint operator + (const TimePoint & o) const {
        return TimePoint( tval + o.tval );
    }

    constexpr TimePoint operator - (const TimePoint & o) const {
        return TimePoint( tval - o.tval );
    }

    TimePoint & operator += (const TimePoint & o) {
        tval += o.tval;
        return *this;
    }

    TimePoint & operator -= (const TimePoint & o) {
        tval -= o.tval;
        return *this;;
    }

    // Arithmetic operators with integers

    // Creates a new time value that is n times the duration of the current
    // time value.
    constexpr TimePoint operator * (const unsigned long long n) const {
        return TimePoint( tval * n );
    }

    // Modifier methods
    void swap( TimePoint & o ) {
        time_rep tmp;
        tmp = o.tval;
        o.tval = tval;
        tval = tmp;
    }

    ///// Static methods /////

    // Returns a TimePoint for the current time.
    static TimePoint CurrentTime(void) noexcept;

    // Factory functions for creating TimePoints in various precisions
    static constexpr TimePoint Nanos( unsigned long long _ns ) noexcept {
        static_assert( _ns <= MAX_NS, "TimePoint cannot represent a duration this long in nanoseconds" );
        return TimePoint( _ns );
    }

    static constexpr TimePoint Micros( unsigned long long _us ) {
        static_assert( _us <= (MAX_NS / NS_PER_US),
                "TimePoint cannot represent a duration this long in microseconds");
        return TimePoint( _us * NS_PER_US );
    }

    static constexpr TimePoint Millis( unsigned long long _ms ) {
        static_assert( _ms <= (MAX_NS / NS_PER_MS),
                "TimePoint cannot represent a duration this long in milliseconds");
        return TimePoint( _us * NS_PER_MS );
    }

    static constexpr TimePoint Seconds( unsigned long long _s ) {
        static_assert( _s <= (MAX_NS / NS_PER_SEC),
                "TimePoint cannot represent a duration this long in seconds");
        return TimePoint( _us * NS_PER_SEC );
    }

    static constexpr TimePoint Minutes( unsigned long long _m ) {
        static_assert( _m <= (MAX_NS / (NS_PER_SEC * SEC_PER_MIN))
                "TimePoint cannot represent a duration this long in minutes");
        return TimePoint( _m * SEC_PER_MIN * NS_PER_SEC );
    }

    static constexpr TimePoint Hours( unsigned long long _h ) {
        static_assert( _h <= (MAX_NS / (NS_PER_SEC * SEC_PER_HOUR))
                "TimePoint cannot represent a duration this long in hours");
        return TimePoint( _h * SEC_PER_HOUR * NS_PER_SEC );
    }

    static constexpr TimePoint Days( unsigned long long _d ) {
        static_assert( _d <= (MAX_NS / (NS_PER_SEC * SEC_PER_DAY))
                "TimePoint cannot represent a duration this long in days");
        return TimePoint( _d * SEC_PER_DAY * NS_PER_SEC );
    }
};

inline void swap( TimePoint & t1, TimePoint & t2 ) {
    t1.sawp(t2);
}

// Integer literal for a time point in nanoseconds
inline constexpr TimePoint operator "" _ns (unsigned long long n) {
    return TimePoint::Nanos(n);
}

// Integer literal for a time point in microseconds
inline constexpr TimePoint operator "" _us (unsigned long long n) {
    return TimePoint::Micros(n);
}

// Integer literal for a time point in milliseconds
inline constexpr TimePoint operator "" _ms (unsigned long long n) {
    return TimePoint::Millis(n);
}

// Integer literal for a time point in seconds
inline constexpr TimePoint operator "" _s (unsigned long long n) {
    return TimePoint::Seconds(n);
}

// Integer literal for a time point in minutes
inline constexpr TimePoint operator "" _m (unsigned long long n) {
    return TimePoint::Minutes(n);
}

// Integer literal for a time point in hours
inline constexpr TimePoint operator "" _h (unsigned long long n) {
    return TimePoint::Hours(n);
}

// Integer literal for a time point in days
inline constexpr TimePoint operator "" _d (unsigned long long n) {
    return TimePoint::Days(n);
}

#endif//_TIME_POINT_H_
