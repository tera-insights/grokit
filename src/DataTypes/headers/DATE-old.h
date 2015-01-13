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
#ifndef _DATE_H_
#define _DATE_H_

#include <string>
#include <stdlib.h>
#include <sstream>
#include <stdio.h>
#include <assert.h>

using namespace std;

/* Datatype to implements dates efficiently. This has to be used
 * in the generated code to deal with dates.
 * Operations are allowed on dates as specified in SQL standard.
 */

class Date;
bool operator < (const Date &d1, const Date &d2);
bool operator <= (const Date &d1, const Date &d2);
bool operator > (const Date &d1, const Date &d2);
bool operator >= (const Date &d1, const Date &d2);
bool operator == (const Date &d1, const Date &d2);
bool operator != (const Date &d1, const Date &d2);
static unsigned int DaysSinceYear0001(Date d);

/* We use a single integer to keep track of date.
 * speeds up comparison a lot since ints are compared
 * numDate stores the year on 16 bits, month on 8 and day on 8 in this order
 * this works on machines 32 bit and higher
 */

#define DAY_MASK 0x000000ff
#define DAY_SHIFT 0
#define MONTH_MASK 0x0000ff00
#define MONTH_SHIFT 8
#define YEAR_MASK 0xffff0000
#define YEAR_SHIFT 16

class Date {
private:
	unsigned int numDate;

public:
	/* Extractor methods */
	unsigned int year () {
		return (numDate & YEAR_MASK) >> YEAR_SHIFT;
	}

	unsigned int month () {
		return (numDate & MONTH_MASK) >> MONTH_SHIFT;
	}

	unsigned int day () {
		return (numDate & DAY_MASK) >> DAY_SHIFT;
	}

	// Default constructor
	Date(void){}

	/* Constructor from year, month, day */
	Date(unsigned int year, unsigned int month, unsigned int day ) {
		FromYMD(year,month,day);
	}

	/* Constructor from string "yyyy/mm/dd". The format is fixed */
	Date (const char *_date){
		FromStringYMD(_date);
	}

	void FromStringYMD(const char *_date) {
		// FIXME: we do not check if the date is valid

		int yy = (_date[0]-'0')*1000+(_date[1]-'0')*100+(_date[2]-'0')*10+(_date[3]-'0');
		int mm = (_date[5]-'0')*10+(_date[6]-'0');
		int dd = (_date[8]-'0')*10+(_date[9]-'0');
		FromYMD(yy,mm,dd);
	}


	void FromYMD(unsigned int year, unsigned int month, unsigned int day ) {

		// FIXME: we do not check if the date is valid (too complicated)
		numDate =  (year<<YEAR_SHIFT) + (month<<MONTH_SHIFT) + (day<<DAY_SHIFT);
	}

	int ToString(char* text){
		return 1+sprintf(text,"%4d/%2d/%2d", year(), month(), day());
	}

	void Print(void){ printf("%4d/%2d/%2d", year(), month(), day()); }

	/* operators */
	friend bool operator < (const Date &d1, const Date &d2) {
		return (d1.numDate<d2.numDate);
	}

	friend bool operator <= (const Date &d1, const Date &d2) {
		return (d1.numDate<=d2.numDate);
	}

	friend bool operator > (const Date &d1, const Date &d2) {
		return (d1.numDate>d2.numDate);
	}

	friend bool operator >= (const Date &d1, const Date &d2) {
		return (d1.numDate>=d2.numDate);
	}

	friend bool operator == (const Date &d1, const Date &d2) {
		return (d1.numDate==d2.numDate);
	}

	friend bool operator != (const Date &d1, const Date &d2) {
		return (d1.numDate!=d2.numDate);
	}

	friend unsigned int Hash(Date d);

	static bool Between (const Date &d, const Date &dl, const Date &dr) {
		return (d.numDate >= dl.numDate && d.numDate <= dr.numDate);
	}
};

/* Functions to extract content from dates */
inline int YEAR(Date _date){
	return _date.year();
}

inline int MONTH(Date _date){
	return _date.month();
}

inline int DAY(Date _date){
	return _date.day();
}

inline void FromString(Date& x, char* text){
	x.FromStringYMD(text);
}

inline int ToString(Date& x, char* text){
	return x.ToString(text);
}

// hash function, jut return the unsigned int inside
inline
unsigned int Hash(Date d){ return d.numDate; }

// compatibility with the other type definitions
typedef Date DATE;

inline
static bool leapCheck (int year) {

  if ((year % 100) == 0) {
    //is century
    if ((year % 400) == 0)
      return true;
  }
  else 
    if ((year % 4) == 0)
      return true;

  return false;
		
}

inline
static unsigned int DaysSinceYear0001(Date d) {

	int leapCount =0;
	int year = d.year();
	int month = d.month();
	int day = d.day();
	
	// For loop to find the number of leap years starting year 0001
	for (int i = 1; i < year; i++) {
		if (leapCheck(i)) 
			leapCount ++;	
	}

	// Check to see if current year is a leap year
	bool thisYear = leapCheck(year);

	//int days = ((year-1800-leapCount)*365) + (leapCount*366);
	int days = ((year-leapCount)*365) + (leapCount*366);

	switch (month) {
		case 1:
			days = days + day;
			break;
		case 2:
			if (day < 29) {
				days = days + 31 + day;
			} else {
				assert(day == 29);
				assert(thisYear);
				days = days + 31 + 29;
			}
			break;
		case 3:
			if (thisYear)
				days = days + 31 + 29 + day;
			else 
				days = days + 31 + 28 + day;
			break;
		case 4: 
			if (thisYear)
				days = days + 31 + 29 + 31 + day;
			else
				days = days + 31 + 28 + 31 + day;
			break;
		case 5:
			if (thisYear)
				days = days + 31 + 29 + 31 + 30 + day;
			else
				days = days + 31 + 28 + 31 + 30 + day;
			break;
		case 6:
			if (thisYear)
				days = days + 31 + 29 + 31 + 30 + 31 + day;
			else 
				days = days + 31 + 28 + 31 + 30 + 31 + day;
			break;
		case 7:
			if (thisYear)
				days = days + 31 + 29 + 31 + 30 + 31 + 30 + day;
			else
				days = days + 31 + 28 + 31 + 30 + 31 + 30 + day;
			break;
		case 8:
			if (thisYear)
				days = days + 31 + 29 + 31 + 30 + 31 + 30 + 31 + day;
			else
				days = days + 31 + 28 + 31 + 30 + 31 + 30 + 31 + day;
			break;
		case 9:
			if (thisYear)
				days = days + 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + day;
			else
				days = days + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + day;
			break;
		case 10:
			if (thisYear)
				days = days + 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + day;
			else
				days = days + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + day;
			break;
		case 11:
			if (thisYear)
				days = days + 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + day;
			else
				days = days + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + day;
			break;
		case 12: 
			if (thisYear)
				days = days + 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + day;
			else
				days = days + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + day;
			break;
	}

	return days;
}

#endif // _DATE_H_
