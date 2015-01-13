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

#ifndef Time_H
#define Time_H

#include "datetimedefs.h"
#include <assert.h>
#include "time.h"

//---------------------------------------------------------------------------
//
// Description:
// CTime is a class for representing the time of day as a 24 hour
// clock time with millisecond precision.
//
// Author: Rob Mueller
// Date: 25-Jul-2001
// Modifications : SS : All code is inlined
//
// Multithread Safe: No (with conditions, see below)
// Mutable Variables: Yes
//
// Copyright:
//	 Copyright (c) 2001 by Robert Mueller.
//
//	 Permission to  use,
//	 copy, modify,  distribute  and  sell  this  software  and  its
//	 documentation for any purpose is hereby granted  without  fee,
//	 provided that the above copyright notice appear in all  copies
//	 and that both that copyright notice and this permission notice
//	 appear in supporting documentation. I make no  representations
//	 about the suitability of this software for any purpose. It  is
//	 provided "as is" without express or implied warranty.
//
// Remarks:
// CTime consists purely of time information. No date
//	information is associated with it. It can be broken down into:
//	* Hour
//	* Minute
//	* Second
//	* Milli-second
//
//  CTime objects can be constructed by specifing either:
//	* hour (0-23), minute (0-59), second (0-59), millisecond (0-999)
//  * total milliseconds past midnight (0-86399999)
//
//	Hours, minutes, seconds and total seconds are not validated, but are
//	asserted in debug mode. The default constructor creates time 00:00:00
//	(eg midnight)
//
//  The current system time can also be obtained with the static method
//	CTime::GetSystemTime()
//
//	Any overflows/underflows wrap the time around
//	eg 00:00:00 - 1 second = 23:59:59
//
//	Internally, time is stored in a Int32 as number of milli-seconds
//	past midnight
//
//---------------------------------------------------------------------------
//
class CTime
{
public:
	// Group=Constructors

	// Description: Constructors.
	// Arguments:
	//  copy - CDate object to create copy from
	//	jd - Julian day number
	//  hour - hour of the day (0-23)
	//	minute - minute of the hour (0-59)
	//	second - seconds of the minute (0-59)
	//	millisecond - milli-seconds of the second (0-999)
	inline CTime(const CTime & copy)
		: m_Time(copy.m_Time) {}
	inline CTime(Int32 millisecPastMidnight = 0)
		: m_Time(millisecPastMidnight) { assert(millisecPastMidnight >= 0); }
	CTime( Int32 hour, Int32 minute, Int32 second, Int32 millisecond = 0 );

	// Description: Returns a CTime instance that represents the current local system time.
	static CTime GetSystemTime();

	// Group=Destructor
	inline ~CTime() {}

	// Group=Public Operators

	// Description: assignment
	inline CTime& operator=(const CTime& assign)
		{ m_Time = assign.m_Time; return *this; }

	// Description: Test if times equal
	bool operator==(const CTime & compare) const;
	// Description: Test if one time less than
	bool operator<(const CTime & compare) const;

	// Description: Add mill-seconds to time and return new time
	// Arguments:
	//	timeOffset - number of milliseconds to add to time (can be negative)
	CTime operator+(Int32 timeOffset) const;

	// Description: Subraction operators
	// Arguments:
	//	timeOffset - Number of milli-seconds to subtract from time (can be negative)
	//	otherTime - Time to subtract. Returns number of mill-seconds between times
	CTime operator-(Int32 timeOffset) const;
	Int32 operator-( const CTime & otherTime) const;

	// Description: Add given number of milli-seconds to current time
	// Arguments:
	//	timeOffset - Number of milli-seconds to add to current time (can be negative)
	CTime& operator+=(Int32 timeOffset);		
	// Description: Subtract given number of milli-seconds from current time
	// Arguments:
	//	timeOffset - Number of milli-seconds to subtract from time (can be negative)
	CTime& operator-=(Int32 timeOffset);

	// Group=Public Member Functions

	// Description: returns hour of the day (0-23)
	Int32 GetHour() const;
	// Description: returns minute of the hour (0-59)
	Int32 GetMinute() const;
	// Description: returns seconds of the minute (0-59)
	Int32 GetSecond() const;
	// Description: returns mill-seconds of the second (0-999)
	Int32 GetMilliSecond() const;

	// Description: Return milli-seconds past midnight
	Int32 GetMilliSecondsPastMidnight() const;

	// Description: Convert internal representation to milli-second number
	void InternalToMSPM() const;
	// Description: Convert internal representation to time parts
	void InternalToTime() const;

	private:
	// Group=Private Member Data

	// Description: The actual time, stored as either milli-seconds
	//	past midnight number or as actual date components depending
	//	on highest bit
	//
	//		Item					Value		Bits		Bit
	//								Range		required	offset 
	//		Storage type			0-1			1			31 
	//		N/A	(always 0)									27-30
	//		Hour					0-23		5			22-26 
	//		Minute					0-59		6			16-21 
	//		Second					0-59		6			10-15 
	//		Milli-second			0-999		10			0-9
	mutable Int32 m_Time;
};

/******************************** Here goes the inline cc part *****************/

#if defined(_MSC_VER)
#pragma warning(disable: 4127)
#endif

// Macro to ensure internal representation is as mill-seconds past midnight
#define REPASMSPM(obj) \
do { \
  if ((obj).m_Time <  0) \
    (obj).InternalToMSPM(); \
} while (0)

// Macro to ensure internal representation is as time components
#define REPASTIME(obj) \
do { \
  if ((obj).m_Time >= 0) \
    (obj).InternalToTime(); \
} while (0)

#define BITOFFSETMILLISECOND 0
#define BITSIZEMILLISECOND 10
#define BITOFFSETSECOND 10
#define BITSIZESECOND 6
#define BITOFFSETMINUTE 16
#define BITSIZEMINUTE 6
#define BITOFFSETHOUR 22
#define BITSIZEHOUR 5

#define MSPERDAY (24*60*60*1000)

// Macro to extract bitfield component at offset of length from v
#define GETCOMPONENT(v, offset, length) ( (v)>>(offset) & ((1<<(length))-1) )

inline
CTime::CTime( Int32 hour, Int32 minute, Int32 second, Int32 millisecond )
{
  m_Time = millisecond + 1000*(second + 60*(minute + 60*hour));
}

inline
CTime CTime::GetSystemTime()
{
  // TODO: Get this to work
  return 0;//CTime(1000*Int32(time(0) % 86400));
}

inline
bool CTime::operator==(const CTime & compare) const
{
  // Ensure the internal representation of both objects is as milli-seconds
  REPASMSPM(*this); REPASMSPM(compare);
  // Simple day compare
  return m_Time == compare.m_Time;
}

inline
bool CTime::operator<(const CTime & compare) const
{
  // Ensure the internal representation of both objects is as milli-seconds
  REPASMSPM(*this); REPASMSPM(compare);
  // Simple milli-second compare
  return m_Time < compare.m_Time;
}

inline
CTime CTime::operator+( Int32 timeOffset) const
{
  CTime ThisTime(*this);
  REPASMSPM(ThisTime);
  ThisTime.m_Time = (ThisTime.m_Time + timeOffset + 2*MSPERDAY) % MSPERDAY;
  assert(ThisTime.m_Time >= 0);
  return ThisTime;
}

inline
CTime CTime::operator-( Int32 timeOffset) const
{
  CTime ThisTime(*this);
  REPASMSPM(ThisTime);
  ThisTime.m_Time = (ThisTime.m_Time - timeOffset + 2*MSPERDAY) % MSPERDAY;
  assert(ThisTime.m_Time >= 0);
  return ThisTime;
}

inline
Int32 CTime::operator-( const CTime& otherTime) const
{
  // Ensure the internal representation of both objects is as milli-seconds
  REPASMSPM(*this); REPASMSPM(otherTime);

  return (m_Time - otherTime.m_Time + 2*MSPERDAY) % MSPERDAY;
}

// Description: Self addition operator

inline
CTime& CTime::operator+=( Int32 timeOffset)
{
  REPASMSPM(*this);
  m_Time = (m_Time + timeOffset + 2*MSPERDAY) % MSPERDAY;
  assert(m_Time >= 0);
  return *this;
}

// Description: Self subtraction operator

inline
CTime& CTime::operator-=( Int32 timeOffset)
{
  REPASMSPM(*this);
  m_Time = (m_Time - timeOffset + 2*MSPERDAY) % MSPERDAY;
  assert(m_Time >= 0);
  return *this;
}

// Description: returns hour of the day (0-23)

inline
Int32 CTime::GetHour() const
{
  REPASTIME(*this);
  Int32 Hour = GETCOMPONENT(m_Time, BITOFFSETHOUR, BITSIZEHOUR);
  assert(Hour >= 0 && Hour < 24);
  return Hour;
}

// Description: returns minute of the hour (0-59)

inline
Int32 CTime::GetMinute() const
{
  REPASTIME(*this);
  Int32 Minute = GETCOMPONENT(m_Time, BITOFFSETMINUTE, BITSIZEMINUTE);
  assert(Minute >= 0 && Minute < 60);
  return Minute;
}

// Description: returns seconds of the minute (0-59)

inline
Int32 CTime::GetSecond() const
{
  REPASTIME(*this);
  Int32 Second = GETCOMPONENT(m_Time, BITOFFSETSECOND, BITSIZESECOND);
  assert(Second >= 0 && Second < 60);
  return Second;
}

// Description: returns mill-seconds of the second (0-999)

inline
Int32 CTime::GetMilliSecond() const
{
  REPASTIME(*this);
  Int32 MilliSecond = GETCOMPONENT(m_Time, BITOFFSETMILLISECOND, BITSIZEMILLISECOND);
  assert(MilliSecond >= 0 && MilliSecond < 1000);
  return MilliSecond;
}

/*
// Description: convert to tstring (overrides CBasicTypeBase::ToTString)

inline
stl::string CTime::ToString() const
{
  // Use ISO YYYY-MM-DD format
  char TimeBuf[12];
  sprintf(TimeBuf, "%04i-%02i-%02i", int(GetYear()), int(GetMonth()), int(GetDay()));
  return stl::string(TimeBuf);
}

// Description: Convert from string (ISO 8601 format YYYY-MM-DD)

inline
void CTime::FromString(const stl::string & timeString)
{
  int Year = 0, Month = 0, Day = 0;
  sscanf(timeString.c_str(), "%i-%i-%i", &Year, &Month, &Day);

  // Return false if not valid time format
  if ((Year == 0) | (Month == 0) | (Day == 0))  {
    return false;
  }

  *this = CTime(Year, Month, Day);

  return true;
}

*/

// Description: Return milli-seconds past midnight

inline
Int32 CTime::GetMilliSecondsPastMidnight() const
{
  REPASMSPM(*this);
  return m_Time;
}


// Description: Convert internal representation to milli-seconds past midnight

inline
void CTime::InternalToMSPM() const
{
  // Should only call this if currently in time representation mode
  assert(m_Time < 0);
  Int32 Hour = GETCOMPONENT(m_Time, BITOFFSETHOUR, BITSIZEHOUR);
  Int32 Minute = GETCOMPONENT(m_Time, BITOFFSETMINUTE, BITSIZEMINUTE);
  Int32 Second = GETCOMPONENT(m_Time, BITOFFSETSECOND, BITSIZESECOND);
  Int32 MilliSecond = GETCOMPONENT(m_Time, BITOFFSETMILLISECOND, BITSIZEMILLISECOND);
  m_Time = MilliSecond + 1000*(Second + 60*(Minute + 60*Hour));
}

// Description: Convert internal representation to time parts

inline
void CTime::InternalToTime() const
{
  // Should only call this if currently in milli-seconds representation mode
  assert(m_Time >= 0);

  // Convert to time parts
  Int32 MilliSecond = m_Time % 1000; m_Time /= 1000;
  Int32 Second = m_Time % 60; m_Time /= 60;
  Int32 Minute = m_Time % 60; m_Time /= 60;
  Int32 Hour = m_Time;

  // copy calculated values
  Int32 TimeRep =
      ((MilliSecond) << BITOFFSETMILLISECOND)
    + ((Second) << BITOFFSETSECOND)
    + ((Minute) << BITOFFSETMINUTE)
    + ((Hour) << BITOFFSETHOUR)
    + (1 << 31);

  m_Time = TimeRep;
}

#endif

