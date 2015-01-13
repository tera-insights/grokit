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
#ifndef Timestamp_H
#define Timestamp_H

#include "date.h"
#include "CTime.h"

//---------------------------------------------------------------------------
//
// Description:
// CTimestamp is a class for representing a data and time
//
// Author: Rob Mueller
// Date: 25-Jul-2001
// Modifications: SS: All code is made inline
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
//
//	CTimestamp is a class for encasulating a point in time, consisting
//	of both date and time information
//
//---------------------------------------------------------------------------
//
class CTimestamp : public CDate, public CTime
{
public:

	// Group=Constructors

	// Description: Constructors.
	// Arguments:
	//  copy - CTimestamp object to create copy from
	//	year - year date represents. Years (00-99) converted to 4 digit year.
	//	month - month of year date represents (1..12)
	//	day - day of month date represents (1..28/29/30/31)
	//	yearDay - day of year date represents (1-365/366 if leap year) 		
	//  hour - hour of the day (0-23)
	//	minute - minute of the hour (0-59)
	//	second - seconds of the minute (0-59)
	//	millisecond - milli-seconds of the second (0-999)
	inline CTimestamp(const CTimestamp & copy)
		: CDate(copy), CTime(copy) {}
	CTimestamp(Int64 extendedJD = 0);
	CTimestamp(Int32 year, Int32 month, Int32 day, Int32 hour, Int32 minute, Int32 second, Int32 millisecond = 0);
	CTimestamp(Int32 year, Int32 yearDay, Int32 hour, Int32 minute, Int32 second, Int32 millisecond = 0);
	CTimestamp(const CDate & date);
	CTimestamp(const CDate & date, const CTime & time);

	// Description: Construct a timestamp representing the current system time
	static CTimestamp GetSystemTimestamp();

	// Group=Destructor

	inline ~CTimestamp() {}

	// Group=Public Operators

	// Description: assignment
	inline CTimestamp& operator=(const CTimestamp& assign)
	{ CDate::operator =(assign); CTime::operator =(assign); return *this; }

	// Description: Test if dates equal
	bool operator==(const CTimestamp & compare) const;
	// Description: Test if one date less than
	bool operator<(const CTimestamp & compare) const;

	// Description: Add milli-seconds to timestamp and return new timestamp
	// Arguments:
	//	timestampOffset - number of milli-seconds to add to timestamp (can be negative)
	CTimestamp operator+(Int64 timestampOffset) const;

	// Description: Subraction operators
	// Arguments:
	//	timestampOffset - Number of milli-seconds to subtract from timestamp (can be negative)
	//	otherTimestamp - Date to subtract. Returns number of milli-seconds between timestamps
	CTimestamp operator-(Int64 timestampOffset) const;
	Int64 operator-( const CTimestamp & otherTimestamp) const;

	// Description: Add given number of milli-seconds to current timestamp
	// Arguments:
	//	timestampOffset - Number of milli-seconds to add to current timestamp (can be negative)
	CTimestamp& operator+=(Int64 timestampOffset);		
	// Description: Subtract given number of milli-seconds from current timestamp
	// Arguments:
	//	timestampOffset - Number of milli-seconds to subtract from timestamp (can be negative)
	CTimestamp& operator-=(Int64 timestampOffset);

	// Group=Public Member Functions

	// Description: Return Julian day number * 86400000 + time
	Int64 GetExtendedJulianDay() const;

	// Rest of methods are from derived CDate and CTime classes


};

/************************* Here goes the inline cc part *************************/

#define MILLISECONDSPERDAY (86400*1000)

inline
CTimestamp::CTimestamp(Int64 extendedJD)
: CDate(Int32(extendedJD / MILLISECONDSPERDAY)), CTime(Int32(extendedJD % MILLISECONDSPERDAY))
{
}

inline
CTimestamp::CTimestamp(Int32 year, Int32 month, Int32 day, Int32 hour, Int32 minute, Int32 second, Int32 millisecond)
: CDate(year, month, day), CTime(hour, minute, second, millisecond)
{
}

inline
CTimestamp::CTimestamp(Int32 year, Int32 yearDay, Int32 hour, Int32 minute, Int32 second, Int32 millisecond)
: CDate(year, yearDay), CTime(hour, minute, second, millisecond)
{
}

inline
CTimestamp::CTimestamp(const CDate & date)
: CDate(date), CTime()
{
}

inline
CTimestamp::CTimestamp(const CDate & date, const CTime & time)
: CDate(date), CTime(time)
{
}

// Description: Test if dates equal

inline
bool CTimestamp::operator==(const CTimestamp & compare) const {
  return CDate::operator ==(compare) && CTime::operator ==(compare);
}

// Description: Test if one date less than

inline
bool CTimestamp::operator<(const CTimestamp & compare) const {
  return
    CDate::operator <(compare)
      ? true
      : CTime::operator <(compare);
}

// Description: Add milli-seconds to timestamp and return new timestamp
// Arguments:
//  timestampOffset - number of milli-seconds to add to timestamp (can be negative)

inline
CTimestamp CTimestamp::operator+(Int64 timestampOffset) const {
  return CTimestamp(GetExtendedJulianDay() + timestampOffset);
}

// Description: Subraction operators
// Arguments:
//  timestampOffset - Number of milli-seconds to subtract from timestamp (can be negative)
//  otherTimestamp - Date to subtract. Returns number of milli-seconds between timestamps

inline
CTimestamp CTimestamp::operator-(Int64 timestampOffset) const {
  return CTimestamp(GetExtendedJulianDay() - timestampOffset);
}

inline
Int64 CTimestamp::operator-( const CTimestamp & otherTimestamp) const {
  return GetExtendedJulianDay() - otherTimestamp.GetExtendedJulianDay();
}

// Description: Add given number of milli-seconds to current timestamp
// Arguments:
//  timestampOffset - Number of milli-seconds to add to current timestamp (can be negative)

inline
CTimestamp& CTimestamp::operator+=(Int64 timestampOffset) {
  *this = *this + timestampOffset;
  return *this;
}

// Description: Subtract given number of milli-seconds from current timestamp
// Arguments:
//  timestampOffset - Number of milli-seconds to subtract from timestamp (can be negative)

inline
CTimestamp& CTimestamp::operator-=(Int64 timestampOffset) {
  *this = *this - timestampOffset;
  return *this;
}

// Description: Return Julian day number * MILLISECONDSPERDAY + time

inline
Int64 CTimestamp::GetExtendedJulianDay() const {
  return Int64(GetJulianDay()) * MILLISECONDSPERDAY + GetMilliSecondsPastMidnight();
}

#endif 
