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
#ifndef DateCalc_H
#define DateCalc_H

#include "datetimedefs.h"
#include <assert.h>


//---------------------------------------------------------------------------
//
// Description:
// DATECalc is a class for doing various Date calculations.
//	It is used by DATE and CSTRTimestamp
//
// Author: Rob Mueller
// Date: 13-Aug-2001
//
// Modifications : SS: Everything is made inline
//
// Remarks:
//
// This class is used by DATE and CSTRTimestamp objects
//	when doing date calculations. It is basically a namespace
//	providing static methods for calculating various parameters
//	of a date It should never be instantiated.
//
// It also allows you to manipulate a couple of global date properties
//	1. What century should 2 digit years be interpreted as
//
// Dates are actually fairly complex items, especially if we consider all
//	the different calendars that have been used, and in some cases, are
//	still used, as well as the dates that different countries changed
//	their calendars. See http://charon.nmsu.edu/~lhuber/leaphist.html for
//	a reasonably comprehensive overview.
//
// The main aims of these date/time classes are:
//	1. Represent modern dates efficiently
//	2. Provide good performance date calculations
//
// Practically then, we consider all dates to use the Gregorian calendar,
//	even if it doesn't make sense to do so. eg. At the 1-Jan-1000 AD,
//	the Gregorian calendar hadn't been created, but we can still create
//	such a date.
//
// We then make the following assumptions about our simple calendar.
//	1. Dates are always Gregorian
//	2. Valid dates are somewhere around 1-Jan-1000 to 1-Jan-3000
//	3. Leap years every 4 years, not every 100, but every 400
//	4. We represent dates using the Julian Period
//
// The Julian Period represents a date by the number of days after
//	1-Jan-4713 BC. It's not specifically related to the Julian Calendar.
//	There are two reasons we use the Julian Period.
//	1. It makes a convenient way to represent a date as a single number
//	2. It's been used by quite a few different organisations (eg NASA)
//	3. Quite a bit of thought has gone into optimising calculations
//		to/from Julian Period numbers and normal dates
// See http://www.faqs.org/faqs/calendars/faq/part2/ and
//	http://www.faqs.org/faqs/calendars/faq/part3/ for more details
//
// For the 'day of week' and 'week of year' calculations, this class
//	uses the ISO 8601 standard where Monday is the first day of the
//	week (1) and the first week of the year is the week that contains
//	Jan-4. See http://www.cl.cam.ac.uk/~mgk25/iso-time.html
//
// Pivot Year:
// The pivot year can be changed on a per application basis by calling
//	DATECalc::SetPivotYear( x ); This call modifies static data and
//	is not thread safe, so it would be advisable to call it once at the
//	earliest possible point of an application. It is not designed to be
//	called multiple times at random intervals from multiple threads.
//
//---------------------------------------------------------------------------
//
class DATECalc
{
	// Remarks: You can't actually create an instance of this class
	DATECalc();
	~DATECalc();

public:
	// Group=Public Member Functions

	// Description: Changes the pivot year used by 2 digit year to 4 digit year conversions
	// Arguments:
	//	pivotYear - The 2 digit year to pivot around
	// Remarks:
	//	When a 2 digit year is supplied to a DATE/CSTRTimestamp method, it is
	//	 automatically converted to a 4 digit year. This method allows you to
	//	 control what years are considered part of the 21st century, and which
	//	 are 19th century. Years <= pivotYear become 20XX while years > pivotYear
	//	 become 19XX
	//	Since this is a static method, it changes the pivot year globally for
	//	 all future instances of DATE and CSTRTimestamp
	static void SetPivotYear( Int32 pivotYear );

	// Description: Returns the current pivot year for 2 digit years
	// Result: The current pivot year
	// Remarks:
	//	See SetPivotYear() for more details
	static Int32 GetPivotYear();

	// Description: Ensure any 2 digit years are converted to 4 digit years
	// Remarks:
	//	This function is used in constructing dates to ensure that 2 digit
	//	 years (00-99) are implicitly converted to a 4 digit year. See class
	//	remarks for details on this conversion. 
	// Result: Returns the 4 digit year using an implicit century conversion.
	static Int32 EnsureYearWithCentury( Int32 year );

	// Description: Calculates the Julian Period from ('year', 'month', 'day')
	//	values or ('year', 'day of year') values
	// Arguments:
	//	year - a 4 digit year
	//	month - month number (1-12)
	//	day - day number (1-31)
	//	dayOfYear - day of year number (1-365/366)
	// Result: The Julian Period of the given date
	static Int32 CalcJulianPeriod(Int32 year, Int32 month, Int32 day);
	static Int32 CalcJulianPeriod(Int32 year, Int32 dayOfYear);

	// Description: A structure that can be filled with details of a date
	//	calculated from the Julian Period
	struct DateS {
		// The year (100-4000)
		Int32	m_Year;
		// The day of the year (1-365, 366 if leap year)
		Int32	m_DayOfYear;
		// The month (1-12). See also MonthOfYearEnum
		Int32	m_Month;
		// The day of the month (1-31)
		Int32	m_Day;
		// True if the year is a leap year
		Int32	m_IsLeapYear;
		// The day of the week (1-7). See also DayOfWeekEnum
		Int32	m_DayOfWeek;
		// The week of the year (1-53)
		Int32	m_WeekOfYear;
		// The year for the week of the year (may be different to
		//	m_Year because last days of Dec can be part of first
		//	week of next year, or first days of Jan can be part
		//	of last week of previous year)
		Int32	m_YearForWeekOfYear;
	};

	// Description: Calculates details of a date from Julian Period
	// Arguments:
	//	julianPeriod - Julian Period number
	//	dateDetails - Structure to fill with details of the date
	//	wantWeekDetails - Set to true to fill in week details in struct
	// Remarks:
	//	Fills in the passed structure with the relevant details.
	//	 By default wantWeekDetails is set to true, but you can set it
	//	 to false to slightly speed up the calculation. Setting it
	//	 to false means the m_DayOfWeek, m_WeekOfYear and
	//	 m_YearForWeekOfYear fields will not be filled in.
	static void CalculateDate(Int32 julianPeriod, DateS & dateDetails, bool wantWeekDetails = true);

	// Description: Returns the number of days in the given month (1-12)
	static Int32 DaysInMonth(Int32 month, bool isLeapYear);

	// Description: Returns true if the year is a leap year
	static Int32 IsLeapYear(Int32 year);

};

/******************** Here starts inline code *********************************/

// Table which maps month number to non-leap number of days in that month
const static Int32 s_DaysPerMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

//---------------------------------------------------------------------------
// Pivot year used for 2 digit - 4 digit conversions
//
static Int32 s_PivotYear = 38;

//---------------------------------------------------------------------------

inline
void DATECalc::SetPivotYear( Int32 pivotYear )
{
  s_PivotYear = pivotYear;
}

//---------------------------------------------------------------------------

inline
Int32 DATECalc::GetPivotYear()
{
  return s_PivotYear;
}

//---------------------------------------------------------------------------

inline
Int32 DATECalc::EnsureYearWithCentury( Int32 year )
{
  if (year < 100) {
    year += 1900 + 100*(year < s_PivotYear);
  }

  return year;
}

//---------------------------------------------------------------------------

inline
Int32 DATECalc::CalcJulianPeriod(Int32 year, Int32 month, Int32 day)
{
  assert(day >= 1 && day <= DaysInMonth(month, IsLeapYear(year) ? true : false));
  // This magical calculation taken from:
  //  http://www.faqs.org/faqs/calendars/faq/part2/
  UInt32 a = (14-month)/12;
  UInt32 y = EnsureYearWithCentury(year)+4800-a;
  UInt32 m = month + 12*a - 3;

  return day + (153*m+2)/5 + y*365 + y/4 - y/100 + y/400 - 32045;
}

inline
Int32 DATECalc::CalcJulianPeriod(Int32 year, Int32 dayOfYear)
{
  // A modification of the calculation above
  UInt32 y =  EnsureYearWithCentury(year)+4799;
  return dayOfYear + y*365 + y/4 - y/100 + y/400 - 31739;
}

//---------------------------------------------------------------------------

inline
void DATECalc::CalculateDate(Int32 julianPeriod, DateS & dateDetails, bool wantWeekDetails)
{
  // This magical calculation taken from:
  //  http://www.faqs.org/faqs/calendars/faq/part2/
  Int32 a = julianPeriod + 32044;
  Int32 b = (4*a+3)/146097;
  Int32 c = a - (b*146097)/4;

  Int32 d = (4*c+3)/1461;
  Int32 e = c - (1461*d)/4;
  Int32 m = (5*e+2)/153;

  Int32 Day = e - (153*m+2)/5 + 1;
  Int32 Month = m + 3 - 12*(m/10);
  Int32 Year = b*100 + d - 4800 + m/10;

  // This formula derived from above JD calculation for 1-Jan-XXXX
  Int32 y = Year+4799;
  Int32 DayOfYear = julianPeriod - (y*365 + y/4 - y/100 + y/400 - 31739);

  // Standard leap year formula
  Int32 IsLeapYear = (Year % 4 == 0) & ((Year % 100 != 0) | (Year % 400 == 0));

  // Fill struct
  dateDetails.m_Year = Year;
  dateDetails.m_DayOfYear = DayOfYear;
  dateDetails.m_Month = Month;
  dateDetails.m_Day = Day;
  dateDetails.m_IsLeapYear = IsLeapYear;

  if (wantWeekDetails) {
    // This magical calculation taken from:
    //  http://www.faqs.org/faqs/calendars/faq/part3/
    Int32 d4 = (julianPeriod+31741 - (julianPeriod % 7)) % 146097 % 36524 % 1461;
    Int32 L  = d4/1460;
    Int32 d1 = ((d4-L) % 365) + L;
    Int32 WeekNumber = d1/7+1;

    Int32 WeekDay = julianPeriod%7 + 1;

    // Week number 1 and 52/52 of a year may actually be in the
    //  previous/next year. Adjust the year number for those cases
    Int32 WeekYear = Year + ((WeekNumber == 1) & (Month == 12)) - ((WeekNumber > 51) & (Month == 1));

    dateDetails.m_DayOfWeek = WeekDay;
    dateDetails.m_WeekOfYear = WeekNumber;
    dateDetails.m_YearForWeekOfYear = WeekYear;
  }

}

//---------------------------------------------------------------------------

inline
Int32 DATECalc::DaysInMonth(Int32 month, bool isLeapYear)
{
  return (s_DaysPerMonth[month] + ((month)==2 && (isLeapYear)));
}

//---------------------------------------------------------------------------

inline
Int32 DATECalc::IsLeapYear(Int32 year)
{
  return (year % 4 == 0) & ((year % 100 != 0) | (year % 400 == 0));
}

#endif

