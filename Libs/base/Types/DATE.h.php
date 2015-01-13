<?
//
//  Copyright 2012 Alin Dobra and Christopher Jermaine, Apache 2.0
//  Copyright 2013, Tera Insights. All rights reserved

// You can redeclare DATE() as long as it is in a namespace

function DATE(){
    $methods = [];
    $constructors = [];
    $functions = [];
?>


//---------------------------------------------------------------------------
//
// Description:
// DATE is a class for representing standard Gregorian calendar dates.
//
// Author: Rob Mueller
// Date: 25-Jul-2001
// Modifications : SS : All code is inlined
//
// Multithread Safe: No (with conditions, see below)
// Mutable Variables: Yes
//
// Copyright:
//     Copyright (c) 2001 by Robert Mueller.
//
//     Permission to  use,
//     copy, modify,  distribute  and  sell  this  software  and  its
//     documentation for any purpose is hereby granted  without  fee,
//     provided that the above copyright notice appear in all  copies
//     and that both that copyright notice and this permission notice
//     appear in supporting documentation. I make no  representations
//     about the suitability of this software for any purpose. It  is
//     provided "as is" without express or implied warranty.
//
// Remarks:
//
// DATE consists purely of date information. No time
//    information is associated with it. It can be broken down into:
//    * Year
//    * Month
//    * Day
//    * Day of Year
//    * Week of Year
//    * Year of "week of year"
//    * Day of Week
//
// See below for details on 'week of year' curiosities
//
// Trying to create an invalid date (eg. 29th Feb in a non-leap year) is
//    not validated, but is asserted in debug mode. The default constructor
//    creates the date with julian day 0. You can't convert this to a
//  year/month/day because it is out of range. The current system date
//    can be obtained with the static method DATE::GetSystemDate(). DATE
//    uses the pivot year set in the DATECalc class to convert 2 digit
//    years to 4 digit years. See DATECalc for more information.
//
// Where possible, DATE uses the International Standard ISO 8601.
//    This is mostly in the definition of the week of year component
//    of a date. See http://www.cl.cam.ac.uk/~mgk25/iso-time.html
//    for some useful docs.
//
// DATE differences can be calculated and a signed Int32 value is
//    used to represent the number of days difference between any two
//    dates.
//
// Implementation notes:
//
// Internally, the date is represented in one of two forms:
//    * A Julian day number
//    * A series of date components: year, day of year, etc
//
// These are stored in a single 32 bit integer.
//    The most significant bit determines if the structure is currently in
//    'Julain day' representation mode or 'date parts' representation mode.
//
// The internal representation can be changed by calling the
//    InternalToDate() or InternalToJD() methods. These methods are
//    const, but do modify the internal values of the class because the
//    m_Date member variable is mutable.
//
// In general, when calling a routine that wants a 'Julian day' value
//    (eg GetJulianDay(), operator+(), etc), the internal representation
//    is first converted to 'Julian day' and the operation performed.
//    Similarily, calling a routine that wants a 'date part' (eg GetYear(),
//    GetMonth(), etc), the internal representation is first converted
//    to 'date parts' and then the appropriate value returned. This seems
//    to give good performance, because you tend to use methods that
//    require a particular representation near each other.
//
// Week of year oddities:
//    Week 01 of a year is per definition the first week that has the Thursday
//    in this year, which is equivalent to the week that contains the fourth
//    day of January. In other words, the first week of a new year is the week
//    that has the majority of its days in the new year
//
//    The week of the year is odd in that week 01 might also contain days from
//    the previous year and the week before week 01 of a year is the last week
//    (52 or 53) of the previous year even if it contains days from the new year.
//    A week starts with Monday (day 1) and ends with Sunday (day 7). For example,
//    the first week of the year 1997 lasts from 1996-12-30 to 1997-01-05
//
// Multithread safety:
//
// Because even const DATE objects can have their internal
//    representation changed, a DATE object is not-thread safe, even
//    as a const read only object! If you know what you're doing, you
//    can ensure you call InternalToDate() or InternalToJD() methods
//    and then only access methods that use that internal representation
//    in a multi-threaded environment
//
//---------------------------------------------------------------------------
//

class DATE
{
public:
    // Group=Constructors

    // Description: Constructors.
    // Arguments:
    //  copy - DATE object to create copy from
    //    jd - Julian day number
    //    year - year date represents. Years (00-99) converted to 4 digit year.
    //    month - month of year date represents (1..12)
    //    day - day of month date represents (1..28/29/30/31)
    //    yearDay - day of year date represents (1-365/366 if leap year)
    inline DATE(const DATE & copy)
        : m_Date(copy.m_Date) {}
<?  $constructors[] = [['base::INT'], true] ?>
    inline DATE(const Int32 julianDay = 0)
        : m_Date(julianDay) { assert(julianDay >= 0); }
<?  $constructors[] = [['base::INT', 'base::INT', 'base::INT'], true] ?>
    DATE(Int32 year, Int32 month, Int32 day);
<?  $constructors[] = [['base::INT', 'base::INT'], true] ?>
    DATE(Int32 year, Int32 yearDay);

<?  $constructors[] = [['BASE::NULL'], true]; ?>
    DATE(const GrokitNull & n);

    // Description: Returns a DATE instance that represents the current local system date.
    static DATE GetSystemDate();

    // Group=Destructor
    inline ~DATE() {}

    // Group=Public Operators

    // Description: assignment
    inline DATE& operator=(const DATE& assign)
        { m_Date = assign.m_Date; return *this; }

    // Description: Test if dates equal
    bool operator==(const DATE & compare) const;
    // Description: Test if one date less than
    bool operator<(const DATE & compare) const;
    bool operator>(const DATE & compare) const;
    bool operator<=(const DATE & compare) const;
    bool operator>=(const DATE & compare) const;

    // Description: Add days to date and return new date
    // Arguments:
    //    dateOffset - number of days to add to date (can be negative)
    DATE operator+(Int32 dateOffset) const;

    // Description: Subraction operators
    // Arguments:
    //    dateOffset - Number of days to subtract from date (can be negative)
    //    otherDate - Date to subtract. Returns number of days between dates
    DATE operator-(Int32 dateOffset) const;
    Int32 operator-( const DATE & otherDate) const;

    // Description: Add given number of days to current date
    // Arguments:
    //    dateOffset - Number of days to add to current date (can be negative)
    DATE& operator+=(Int32 dateOffset);
    // Description: Subtract given number of days from current date
    // Arguments:
    //    dateOffset - Number of days to subtract from date (can be negative)
    DATE& operator-=(Int32 dateOffset);

    // Group=Public Member Functions

<?  $methods[] = [ 'IsNull', [], 'BASE::BOOL', true ]; ?>
    bool IsNull(void) const;

    // Description: Returns the year represented by this date (1500-2500)
<?  $methods[] = [ 'GetYear', [], 'base::INT', true ];    ?>
    Int32 GetYear() const;
    // Description: Returns the quarter represented by this date (1-4)
<?  $methods[] = [ 'GetQuarter', [], 'base::INT', true ];    ?>
    Int32 GetQuarter() const;
    // Description: Returns the month in the year represented by this date (1-12)
<?  $methods[] = [ 'GetMonth', [], 'base::INT', true ];    ?>
    Int32 GetMonth() const;
    // Description: Returns the day in the month represented by this date (1-31)
<?  $methods[] = [ 'GetDay', [], 'base::INT', true ];    ?>
    Int32 GetDay() const;
    // Description: Returns the day of the year represented by this date (1-365, 366 if leap year)
<?  $methods[] = [ 'GetDayOfYear', [], 'base::INT', true ];    ?>
    Int32 GetDayOfYear() const;

    // Description: Returns the week of the year reprsented by this date (1-53).
    //    See DATE class description for more details
<?  $methods[] = [ 'GetWeekOfYear', [], 'base::INT', true ];    ?>
    Int32 GetWeekOfYear() const;
    // Description: Returns the year of for the current week of the year. This
    //    may be different to GetYear() for certain cross-over days.
    //    See DATE class description for more details
<?  $methods[] = [ 'GetYearForWeekOfYear', [], 'base::INT', true ];    ?>
    Int32 GetYearForWeekOfYear() const;
    // Description: Returns the weekday of the week represented by this date
    //    (1-7) => (Monday-Sunday)
<?  $methods[] = [ 'GetDayOfWeek', [], 'base::INT', true ];    ?>
    Int32 GetDayOfWeek() const;

    // Description: Return Julian day number
<?  $methods[] = [ 'GetJulianDay', [], 'base::INT', true ];    ?>
    Int32 GetJulianDay() const;

    // Description:
    //stl::string DATE::ToString() const;
    //bool FromString(const stl::string & dateString);
    int ToString(char* text) const;
    void FromString(const char* dateString);

    // Description: Convert internal representation to Julian day number
    void InternalToJD() const;
    // Description: Convert internal representation to date parts
    void InternalToDate() const;

    // Below functions added by me : SS
        /* Constructor from string "yyyy/mm/dd". The format is fixed */
<?  $constructors[] = [['base::STRING_LITERAL'], true] ?>
        DATE (const char *_date){
                FromStringYMD(_date);
        }

        void FromStringYMD(const char *_date) {
                // FIXME: we do not check if the date is valid

                int yy = (_date[0]-'0')*1000+(_date[1]-'0')*100+(_date[2]-'0')*10+(_date[3]-'0');
                int mm = (_date[5]-'0')*10+(_date[6]-'0');
                int dd = (_date[8]-'0')*10+(_date[9]-'0');
                FromYMD(yy,mm,dd);
        }

        void FromYMD(Int32 year, Int32 month, Int32 day ) {

                // FIXME: we do not check if the date is valid (too complicated)
        m_Date = (Int32)DATECalc::CalcJulianPeriod(year, month, day);
        }

    void Print(void){ printf("%4d/%2d/%2d", GetYear(), GetMonth(), GetDay()); }

    private:
    // Group=Private Member Data

    // Description: The actual date, stored as either Julian day number or
    //    as actual date components depending on highest bit
    //
    //        Item                    Value        Bits        Bit
    //                                Range        required    offset
    //        Storage type            0-1            1            31
    //        Is leap year?           0-1            1            30
    //        Week year difference    -1 - 1         2            28-29
    //        Month                   1-12           4            24-27
    //        Day of week             1-7            3            21-23
    //        Day                     1-31           5            16-20
    //        Week of year            1-53           6            10-15
    //        Year                    1500-2500      10            0-9
    mutable Int32 m_Date;
};

/******************* Here goes the inline cc code **************************/

#if defined(_MSC_VER)
#pragma warning(disable: 4127)
#endif

// Macro to ensure internal representation is as Julian day
#define REPASJD(obj) \
do { \
  if ((obj).m_Date <  0) \
    (obj).InternalToJD(); \
} while (0)

// Macro to ensure internal representation is as date components
#define REPASDATE(obj) \
do { \
  if ((obj).m_Date >= 0) \
    (obj).InternalToDate(); \
} while (0)

#define BITOFFSETYEAR 0
#define BITSIZEYEAR 10
#define BITOFFSETWEEKOFYEAR 10
#define BITSIZEWEEKOFYEAR 6
#define BITOFFSETDAY 16
#define BITSIZEDAY 5
#define BITOFFSETDAYOFWEEK 21
#define BITSIZEDAYOFWEEK 3
#define BITOFFSETMONTH 24
#define BITSIZEMONTH 4
#define BITOFFSETWEEKYEARDIF 28
#define BITSIZEWEEKYEARDIF 2
#define BITOFFSETISLEAPYEAR 30
#define BITSIZEISLEAPYEAR 1

// We're using 10 bits to store the year (1024 years). Set the minimum year
#define MINIMUMYEAR 1500

// Macro to extract bitfield component at offset of length from v
#define GETCOMPONENT(v, offset, length) ( (v)>>(offset) & ((1<<(length))-1) )
//>

// Table which maps month number to non-leap year day number of the first day of that month
const static Int32 s_YearDayFromMonth[] = { 0, 1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };

inline
DATE::DATE(Int32 year, Int32 month, Int32 day)
{
  m_Date = (Int32)DATECalc::CalcJulianPeriod(year, month, day);
}

inline
DATE::DATE(Int32 year, Int32 dayOfYear)
{
  m_Date = (Int32)DATECalc::CalcJulianPeriod(year, dayOfYear);
}

inline
DATE::DATE(const GrokitNull & n) {
    // All 1s is never valid, as that would have more weeks than in a year,
    // etc.
    m_Date = 0xFFFFFFFF;
}

inline
bool DATE::IsNull(void) const {
    return m_Date == 0xFFFFFFFF;
}

inline
DATE DATE::GetSystemDate()
{
  // TODO: Get this to work
  return 0;//DATE(Int32(time(0) / 86400));
}

inline
bool DATE::operator==(const DATE & compare) const
{
  // Ensure the internal representation of both objects is as days
  REPASJD(*this); REPASJD(compare);
  // Simple day compare
  return m_Date == compare.m_Date;
}

inline
bool DATE::operator<(const DATE & compare) const
{
  // Ensure the internal representation of both objects is as days
  REPASJD(*this); REPASJD(compare);
  // Simple day compare
  return m_Date < compare.m_Date;
}

inline
bool DATE::operator>(const DATE & compare) const
{
  // Ensure the internal representation of both objects is as days
  REPASJD(*this); REPASJD(compare);
  // Simple day compare
  return m_Date > compare.m_Date;
}
inline
bool DATE::operator<=(const DATE & compare) const
{
  // Ensure the internal representation of both objects is as days
  REPASJD(*this); REPASJD(compare);
  // Simple day compare
  return m_Date <= compare.m_Date;
}

inline
bool DATE::operator>=(const DATE & compare) const
{
  // Ensure the internal representation of both objects is as days
  REPASJD(*this); REPASJD(compare);
  // Simple day compare
  return m_Date >= compare.m_Date;
}

inline
DATE DATE::operator+( Int32 dateOffset) const
{
  return DATE(GetJulianDay() + dateOffset);
}

inline
DATE DATE::operator-( Int32 dateOffset) const
{
  return DATE(GetJulianDay() - dateOffset);
}

inline
Int32 DATE::operator-( const DATE& otherDate) const
{
  // Ensure the internal representation of both objects is as days
  REPASJD(*this); REPASJD(otherDate);

  return m_Date - otherDate.m_Date;
}

// Description: Self addition operator

inline
DATE& DATE::operator+=( Int32 dateOffset)
{
  REPASJD(*this);
  m_Date += dateOffset;
  assert(m_Date >= 0);
  return *this;
}

// Description: Self subtraction operator

inline
DATE& DATE::operator-=( Int32 dateOffset)
{
  REPASJD(*this);
  m_Date -= dateOffset;
  assert(m_Date >= 0);
  return *this;
}

// Description: Returns the year represented by this date (1500-2500)

inline
Int32 DATE::GetYear() const
{
  REPASDATE(*this);
  Int32 Year = GETCOMPONENT(m_Date, BITOFFSETYEAR, BITSIZEYEAR) + MINIMUMYEAR;
  assert(Year >= MINIMUMYEAR && Year <= MINIMUMYEAR + 1000);
  return Year;
}

// Description: Returns the quarter represented by this date (1-4)

inline
Int32 DATE::GetQuarter() const
{
  REPASDATE(*this);
  Int32 Quarter = GETCOMPONENT(m_Date, BITOFFSETMONTH, BITSIZEMONTH) / 3;
  assert(Quarter >= 1 && Quarter <= 4);
  return Quarter;
}

// Description: returns the month in the year represented by this date (1-12)

inline
Int32 DATE::GetMonth() const
{
  REPASDATE(*this);
  Int32 Month = GETCOMPONENT(m_Date, BITOFFSETMONTH, BITSIZEMONTH);
  assert(Month >= 1 && Month <= 12);
  return Month;
}

// Description: returns the day in the month represented by this date (1-31)
inline
Int32 DATE::GetDay() const
{
  REPASDATE(*this);
  Int32 Day = GETCOMPONENT(m_Date, BITOFFSETDAY, BITSIZEDAY);
  assert(Day >= 1 && Day <= DATECalc::DaysInMonth(GetMonth(), GETCOMPONENT(m_Date, BITOFFSETISLEAPYEAR, BITSIZEISLEAPYEAR) ? true : false));
  return Day;
}

// Description: returns the day of the year represented by this date (1-365, 366 if leap year)

inline
Int32 DATE::GetDayOfYear() const
{
  REPASDATE(*this);
  Int32 Month = GETCOMPONENT(m_Date, BITOFFSETMONTH, BITSIZEMONTH); // 1 - 12
  Int32 DayOfYear = s_YearDayFromMonth[Month]
    + GETCOMPONENT(m_Date, BITOFFSETDAY, BITSIZEDAY)
    + (GETCOMPONENT(m_Date, BITOFFSETISLEAPYEAR, BITSIZEISLEAPYEAR) & (Month > 2)) - 1;
  assert(DayOfYear >= 1 && DayOfYear <= 365 + GETCOMPONENT(m_Date, BITOFFSETISLEAPYEAR, BITSIZEISLEAPYEAR));
  return DayOfYear;
}

// Description: returns the week of the year reprsented by this date (1-52)

inline
Int32 DATE::GetWeekOfYear() const
{
  REPASDATE(*this);
  Int32 WeekOfYear = GETCOMPONENT(m_Date, BITOFFSETWEEKOFYEAR, BITSIZEWEEKOFYEAR);
  assert(WeekOfYear >= 1 && WeekOfYear <= 53);
  return WeekOfYear;
}

// Description: Returns the year of for the current week of the year. This
//  may be different to GetYear() for certain cross-over days.
//  See DATE class description for more details

inline
Int32 DATE::GetYearForWeekOfYear() const
{
  REPASDATE(*this);
  Int32 Year = GETCOMPONENT(m_Date, BITOFFSETYEAR, BITSIZEYEAR) + MINIMUMYEAR;
  Int32 YearOffset = GETCOMPONENT(m_Date, BITOFFSETWEEKYEARDIF, BITSIZEWEEKYEARDIF) - 1;
  assert(YearOffset >= -1 && YearOffset <= 1);
  return Year + YearOffset;
}

// Description: Returns the weekday of the week represented by this date (1-7) => (Monday-Sunday)

inline
Int32 DATE::GetDayOfWeek() const
{
  REPASDATE(*this);
  Int32 DayOfWeek = GETCOMPONENT(m_Date, BITOFFSETDAYOFWEEK, BITSIZEDAYOFWEEK);
  assert(DayOfWeek >= 1 && DayOfWeek <= 7);
  return DayOfWeek;
}


// Description: convert to tstring (overrides CBasicTypeBase::ToTString)

inline
int DATE::ToString(char* text) const
{
  // Use ISO YYYY/MM/DD format
  return 1+sprintf(text, "%04d/%02d/%02d", int(GetYear()), int(GetMonth()), int(GetDay()));
}

// Description: Convert from string (ISO 8601 format YYYY-MM-DD)

inline
void DATE::FromString(const char* dateString)
{
  int Year = 0, Month = 0, Day = 0;
  sscanf(dateString, "%d/%d/%d", &Year, &Month, &Day);

  // assert if not valid date format
  assert (!((Year == 0) | (Month == 0) | (Day == 0)));

  *this = DATE(Year, Month, Day);
}


// Description: Return Julian day number

inline
Int32 DATE::GetJulianDay() const
{
  REPASJD(*this);
  return m_Date;
}

// Description: Convert internal representation to Julian day number

inline
void DATE::InternalToJD() const
{
  // Should only call this if currently in date representation mode
  assert(m_Date < 0);
  Int32 Year = GETCOMPONENT(m_Date, BITOFFSETYEAR, BITSIZEYEAR) + MINIMUMYEAR;
  Int32 Month = GETCOMPONENT(m_Date, BITOFFSETMONTH, BITSIZEMONTH);
  Int32 Day = GETCOMPONENT(m_Date, BITOFFSETDAY, BITSIZEDAY);
  m_Date = DATECalc::CalcJulianPeriod(Year, Month, Day);
}

// Description: Convert internal representation to date parts

inline
void DATE::InternalToDate() const
{
  // Should only call this if currently in days representation mode
  assert(m_Date >= 0);

  // Convert to date parts
  DATECalc::DateS ConvDate;
  DATECalc::CalculateDate(m_Date, ConvDate);

  // Copy calculated values
  Int32 DateRep =
      ((ConvDate.m_Year - MINIMUMYEAR) << BITOFFSETYEAR)
    + ((ConvDate.m_WeekOfYear) << BITOFFSETWEEKOFYEAR)
    + ((ConvDate.m_Day) << BITOFFSETDAY)
    + ((ConvDate.m_DayOfWeek) << BITOFFSETDAYOFWEEK)
    + ((ConvDate.m_Month) << BITOFFSETMONTH)
    + ((ConvDate.m_YearForWeekOfYear - ConvDate.m_Year + 1) << BITOFFSETWEEKYEARDIF)
    + ((ConvDate.m_IsLeapYear) << BITOFFSETISLEAPYEAR)
    + (1 << 31);

  m_Date = DateRep;
}

<?  ob_start(); // global content ?>

//>
inline
void FromString(@type& x, const char* text){
  //SS x.FromString(text);
  x.FromStringYMD(text);
}

inline
int ToString(const @type& x, char* text){
  return x.ToString(text);
}

inline 
int64_t ClusterValue(const @type& x){
  // correct it to Unix day
  return x.GetJulianDay();
}

inline
void ToJson(const @type& src, Json::Value & dest ) {
    char buffer[12];
    src.ToString(buffer);
    dest = buffer;
}

inline
void FromJson(const Json::Value & src, @type & dest ) {
    dest.FromStringYMD(src.asCString());
}

// Hash function
<?  $functions[] = ['Hash', ['@type'], 'BASE::BIGINT', true, true ]; ?>
inline uint64_t Hash( const @type val ) {
    return val.GetJulianDay();
}

// Deep copy function
inline
void Copy( @type& to, const @type& from ) {
    to = from;
}

<?  $functions[] = ['IsNull', ['@type'], 'BASE::BOOL', true, true ]; ?>
inline
bool IsNull( const @type & d ) {
    return d.IsNull();
}

#ifdef _HAS_STD_HASH
// C++11 STL-compliant hash struct specialization

namespace std {

template <>
class hash<@type> {
public:
    size_t operator () (const @type& key) const {
        return Hash(key);
    }
};

}
#endif // _HAS_STD_HASH

<?  $globalContent = ob_get_clean(); ?>

<?

    return array(
        'kind'              => 'TYPE',
        "system_headers"    => array ("assert.h"),
        "user_headers"      => array ( "datetimedefs.h", "datecalc.h", "Constants.h", "Config.h" ),
        "complex"           => false,
        // The + and - operators for DATE do not follow the normal rule for those operators, so
        // they must be defined separately.
        'binary_operators'  => [ '==', '!=', '>', '<', '>=', '<=' ],
        'global_content'    => $globalContent,
        'constructors'      => $constructors,
        'methods'           => $methods,
        'functions'         => $functions,
        'properties'        => [ 'clusterable' ],
        'describe_json'     => DescribeJson('date', DescribeJsonStatic([ 'format' => 'YYYY/MM/DD'])),
        'extras'            => [ 'size.bytes' => 4 ]
    );

} // end of function

// Define + and - operators
// DATE + INT = DATE
declareOperator( '+', [ 'base::DATE', 'base::INT' ],
    function($args, $targs = []) {
        $dateType = lookupType( 'base::DATE' );
        $intType = lookupType( 'base::INT' );

        return array(
            'kind' => 'OPERATOR',
            'input' => [ $dateType, $intType ],
            'result' => $dateType,
            'deterministic' => true,
        );
    }
);

// DATE - INT = DATE
declareOperator( '-', [ 'base::DATE', 'base::INT' ],
    function($args, $targs = []) {
        $dateType = lookupType( 'base::DATE' );
        $intType = lookupType( 'base::INT' );

        return array(
            'kind' => 'OPERATOR',
            'input' => [ $dateType, $intType ],
            'result' => $dateType,
            'deterministic' => true,
        );
    }
);

// DATE - DATE = INT
declareOperator( '-', [ 'base::DATE', 'base::DATE' ],
    function($args, $targs = []) {
        $dateType = lookupType( 'base::DATE' );
        $intType = lookupType( 'base::INT' );

        return array(
            'kind' => 'OPERATOR',
            'input' => [ $dateType, $dateType ],
            'result' => $intType,
            'deterministic' => true,
        );
    }
);
?>



