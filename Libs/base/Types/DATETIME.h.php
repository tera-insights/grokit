
<?
// Copyright 2014 Tera Insights, LLC. All Rights Reserved.

function DATETIME() {
    $methods = [];
    $constructors = [];
    $functions = [];
    $bin_operators = [];
    $libraries = [];

    $system_headers = [ 'cinttypes', 'ctime', 'cstdio' ];

    $system_headers[] = 'boost/date_time/posix_time/posix_time.hpp';
    $system_headers[] = 'boost/date_time/gregorian/gregorian.hpp';
    $system_headers[] = 'boost/date_time/dst_rules.hpp';
    $libraries[] = 'boost_date_time';

    $globalContent = "";
?>

class DATETIME {
private:
    static constexpr const int32_t TM_YEAR_OFFSET = 1900;

    static constexpr const int32_t INVALID_DTIME = -1;

    static constexpr const int32_t S_PER_MIN = 60;
    static constexpr const int32_t S_PER_HR = S_PER_MIN * 60;
    static constexpr const int32_t S_PER_DAY = S_PER_HR * 24;

    // Each month is encoded in 5 bits, with january in the least significant
    // bits
    static constexpr const uint64_t DAYS_PER_MONTH = 0x0ffbfefffdff7f9f;
    static constexpr const uint8_t DPM_MASK = 0x1F;
    static constexpr const uint8_t DPM_BITS = 5;

    int32_t ctime;

    void Set(
        int16_t     year,
        int8_t      month,
        int8_t      day,
        int8_t      hour,
        int8_t      minute,
        int8_t      second
    );

public:
<?  $constructors[] = [[], true];  ?>
    constexpr DATETIME(void);

<?  $constructors[] = [['BASE::INT'], true];  ?>
    constexpr DATETIME(const int32_t _ctime);

<?  $constructors[] = [['BASE::NULL'], true];  ?>
    constexpr DATETIME(const GrokitNull & nullval);

<?  $constructors[] = [['BASE::SMALLINT', 'BASE::BYTE', 'BASE::BYTE', 'BASE::BYTE', 'BASE::BYTE', 'BASE::BYTE'], true];  ?>
    DATETIME(
        int16_t    year,
        int8_t     month,
        int8_t     day,
        int8_t     hour,
        int8_t     minute,
        int8_t     second
    );

<?  $constructors[] = [['BASE::STRING_LITERAL'], true]; ?>
    DATETIME(const char * str);

    // Accessors for portions of the date
<?  $methods[] = ['Year', [], 'BASE::SMALLINT', true];  ?>
    int16_t Year(void) const;
<?  $methods[] = ['Month', [], 'BASE::BYTE', true];  ?>
    int8_t  Month(void) const;
<?  $methods[] = ['Day', [], 'BASE::BYTE', true];  ?>
    int8_t  Day(void) const;
<?  $methods[] = ['Hour', [], 'BASE::BYTE', true];  ?>
    int8_t  Hour(void) const;
<?  $methods[] = ['Minute', [], 'BASE::BYTE', true];  ?>
    int8_t  Minute(void) const;
<?  $methods[] = ['Second', [], 'BASE::BYTE', true];  ?>
    int8_t  Second(void) const;

<?  $methods[] = ['DayOfWeek', [], 'BASE::BYTE', true];  ?>
    int8_t  DayOfWeek(void) const;
<?  $methods[] = ['DayOfYear', [], 'BASE::SMALLINT', true];  ?>
    int16_t DayOfYear(void) const;

    // Returns the number of days in the date's month
<?  $methods[] = ['DaysInMonth', [], 'BASE::BYTE', true]; ?>
    int8_t  DaysInMonth(void) const;

<?  $methods[] = ['AsDays', [], 'BASE::INT', true] ?>
    constexpr int32_t AsDays(void) const;

<?  $methods[] = ['AsHours', [], 'BASE::INT', true]; ?>
    constexpr int32_t AsHours(void) const;

<?  $methods[] = ['AsMinutes', [], 'BASE::INT', true];  ?>
    constexpr int32_t AsMinutes(void) const;

<?  $methods[] = ['AsSeconds', [], 'BASE::INT', true];  ?>
    constexpr int32_t AsSeconds(void) const;

<?  $methods[] = ['IsValid', [], 'BASE::BOOL', true];  ?>
    constexpr bool IsValid(void) const;
<?  $methods[] = ['IsNull', [], 'BASE::BOOL', true];  ?>
    constexpr bool IsNull(void) const;

<?  $methods[] = ['IsDST', [], 'BASE::BOOL', true]; ?>
    bool IsDST(void) const;
<?  $methods[] = ['IsDSTBoundary', [], 'BASE::BOOL', true]; ?>
    bool IsDSTBoundary(void) const;

<?  $methods[] = ['GetDSTBoundary', [], 'BASE::BYTE', true]; ?>
    // Returns -1 if starting DST this day,
    // 1 if ending DST this day,
    // and 0 otherwise
    int8_t GetDSTBoundary(void) const;

    // Returns a new DATETIME with the time component set to 00:00:00
    // and the day set to 1
<?  $methods[] = ['ByMonth', [], 'BASE::DATETIME', true]; ?>
    DATETIME ByMonth(void) const;

    // Comparisons
<?  $bin_operators[] = '==';  ?>
    constexpr bool operator ==(const DATETIME & o) const;
<?  $bin_operators[] = '!=';  ?>
    constexpr bool operator !=(const DATETIME & o) const;
<?  $bin_operators[] = '<';  ?>
    constexpr bool operator <(const DATETIME & o) const;
<?  $bin_operators[] = '>';  ?>
    constexpr bool operator >(const DATETIME & o) const;
<?  $bin_operators[] = '<=';  ?>
    constexpr bool operator <=(const DATETIME & o) const;
<?  $bin_operators[] = '>=';  ?>
    constexpr bool operator >=(const DATETIME & o) const;

    constexpr bool operator ==(const int32_t secs) const;
    constexpr bool operator !=(const int32_t secs) const;
    constexpr bool operator <(const int32_t secs) const;
    constexpr bool operator >(const int32_t secs) const;
    constexpr bool operator <=(const int32_t secs) const;
    constexpr bool operator >=(const int32_t secs) const;

    // Adding / subtracting dates
    constexpr DATETIME operator +(const int32_t secs) const;
    constexpr DATETIME operator -(const int32_t secs) const;
    constexpr int32_t operator -(const DATETIME & o) const;

    void FromString( const char * str );
    int ToString( char * buffer ) const;

    void FromJson( const Json::Value & );
    void ToJson( Json::Value & ) const;

    // Static Members //

    static constexpr int8_t DaysInMonth(int16_t year, int8_t month);

private:
    static constexpr int8_t GetDPM(int8_t month) {
        return (DAYS_PER_MONTH >> (month * DPM_BITS)) & DPM_MASK;
    }
};


inline
constexpr int8_t DATETIME::DaysInMonth(int16_t year, int8_t month) {
    return (((month - 1) != 1) || (year % 4 != 0) || (year % 100 == 0 && year % 400 != 0)) ?
        GetDPM(month - 1) : GetDPM(month - 1) + 1;
}

inline
void DATETIME :: Set( int16_t year, int8_t month, int8_t day, int8_t hour, int8_t minute, int8_t second ) {


    tm dtime;
    dtime.tm_year = year - TM_YEAR_OFFSET;
    dtime.tm_mon = month - 1;  // tm struct month is in range [0, 11]
    dtime.tm_mday = day;
    dtime.tm_hour = hour;
    dtime.tm_min = minute;
    dtime.tm_sec = second;
    dtime.tm_isdst = -1;

    time_t c_time = mktime( &dtime );
    ctime = static_cast<int32_t>(c_time);
}

inline constexpr
DATETIME :: DATETIME(void) : ctime(INVALID_DTIME)
{ }

inline constexpr
DATETIME :: DATETIME(const int32_t _ctime) : ctime(_ctime)
{ }

inline constexpr
DATETIME :: DATETIME(const GrokitNull & nullval) : ctime(INVALID_DTIME)
{ }

inline
DATETIME :: DATETIME(const char * str)
{
    this->FromString(str);
}

inline
DATETIME :: DATETIME( int16_t year, int8_t month, int8_t day, int8_t hour, int8_t minute, int8_t second ) {
    Set(year, month, day, hour, minute, second);
}

inline
int16_t DATETIME :: Year(void) const {
    auto temp = boost::posix_time::from_time_t(static_cast<time_t>(ctime));
    return temp.date().year();
}

inline
int8_t DATETIME :: Month(void) const {
    auto temp = boost::posix_time::from_time_t(static_cast<time_t>(ctime));
    return temp.date().month();
}

inline
int8_t DATETIME :: Day(void) const {
    auto temp = boost::posix_time::from_time_t(static_cast<time_t>(ctime));
    return temp.date().day();
}

inline
int8_t DATETIME :: Hour(void) const {
    auto temp = boost::posix_time::from_time_t(static_cast<time_t>(ctime));
    return temp.time_of_day().hours();
}

inline
int8_t DATETIME :: Minute(void) const {
    auto temp = boost::posix_time::from_time_t(static_cast<time_t>(ctime));
    return temp.time_of_day().minutes();
}

inline
int8_t DATETIME :: Second(void) const {
    auto temp = boost::posix_time::from_time_t(static_cast<time_t>(ctime));
    return temp.time_of_day().seconds();
}

inline
int8_t DATETIME :: DayOfWeek(void) const {
    auto temp = boost::posix_time::from_time_t(static_cast<time_t>(ctime));
    return temp.date().day_of_week();
}

inline
int16_t DATETIME :: DayOfYear(void) const {
    auto temp = boost::posix_time::from_time_t(static_cast<time_t>(ctime));
    return temp.date().day_of_year();
}

inline
int8_t DATETIME :: DaysInMonth(void) const {
    auto temp = boost::posix_time::from_time_t(static_cast<time_t>(ctime));
    auto ymd = temp.date().year_month_day();
    return DATETIME::DaysInMonth(ymd.year, ymd.month);
}

inline
bool DATETIME :: IsDST(void) const {
    auto temp = boost::posix_time::from_time_t(static_cast<time_t>(ctime));
    return boost::posix_time::us_dst::local_is_dst(temp.date(), temp.time_of_day()) ==
        boost::date_time::is_in_dst;
}

inline
bool DATETIME :: IsDSTBoundary(void) const {
    auto dtime = boost::posix_time::from_time_t(static_cast<time_t>(ctime));
    return boost::posix_time::us_dst::is_dst_boundary_day(dtime.date());
}

inline
int8_t DATETIME :: GetDSTBoundary(void) const {
    using boost::posix_time::us_dst;

    auto dtime = boost::posix_time::from_time_t(static_cast<time_t>(ctime));
    auto date = dtime.date();

    if (date == us_dst::local_dst_start_day(date.year()))
        return -1;
    else if (date == us_dst::local_dst_end_day(date.year()))
        return 1;
    else
        return 0;
}

inline constexpr
int32_t DATETIME :: AsDays(void) const {
    return ctime / S_PER_DAY;
}

inline constexpr
int32_t DATETIME :: AsHours(void) const {
    return ctime / S_PER_HR;
}

inline constexpr
int32_t DATETIME :: AsMinutes(void) const {
    return ctime / S_PER_MIN;
}

inline constexpr
int32_t DATETIME :: AsSeconds(void) const {
    return ctime;
}

inline constexpr
bool DATETIME :: IsValid(void) const {
    return ctime >= 0;
}

inline constexpr
bool DATETIME :: IsNull(void) const {
    return ctime < 0; //>
}

inline
DATETIME DATETIME :: ByMonth(void) const {
    auto temp = boost::posix_time::from_time_t(static_cast<time_t>(ctime));
    auto date = temp.date();
    return DATETIME(date.year(), date.month(), 1, 0, 0, 0);
}

inline constexpr
bool DATETIME :: operator ==(const DATETIME & o) const {
    return ctime == o.ctime;
}

inline constexpr
bool DATETIME :: operator !=(const DATETIME & o) const {
    return ctime != o.ctime;
}

inline constexpr
bool DATETIME :: operator <(const DATETIME & o) const {
    return ctime < o.ctime;
}

inline constexpr
bool DATETIME :: operator >(const DATETIME & o) const {
    return ctime > o.ctime;
}

inline constexpr
bool DATETIME :: operator <=(const DATETIME & o) const {
    return ctime <= o.ctime;
}

inline constexpr
bool DATETIME :: operator >=(const DATETIME & o) const {
    return ctime >= o.ctime;
}

inline constexpr
bool DATETIME :: operator ==(const int32_t secs) const {
    return ctime == secs;
}

inline constexpr
bool DATETIME :: operator !=(const int32_t secs) const {
    return ctime != secs;
}

inline constexpr
bool DATETIME :: operator <(const int32_t secs) const {
    return ctime < secs;
}

inline constexpr
bool DATETIME :: operator >(const int32_t secs) const {
    return ctime > secs;
}

inline constexpr
bool DATETIME :: operator <=(const int32_t secs) const {
    return ctime <= secs;
}

inline constexpr
bool DATETIME :: operator >=(const int32_t secs) const {
    return ctime >= secs;
}

inline constexpr
DATETIME DATETIME :: operator +(const int32_t secs) const {
    return DATETIME(ctime + secs);
}

inline constexpr
DATETIME DATETIME :: operator -(const int32_t secs) const {
    return DATETIME(ctime - secs);
}

inline constexpr
int32_t DATETIME :: operator -( const DATETIME & o ) const {
    return ctime - o.ctime;
}

inline
void DATETIME :: FromString( const char * str ) {
    using namespace boost::posix_time;

/*
    short year;
    char month;
    char day;
    char hour;
    char minute;
    char second;

    sscanf(str, "%hd-%hhd-%hhd %hhd:%hhd:%hhd", &year, &month, &day, &hour, &minute, &second);
*/

    ptime posixTime = time_from_string(str);
    ptime epoch(boost::gregorian::date(1970, 1, 1));
    time_duration::sec_type x = (posixTime - epoch).total_seconds();

    ctime = int32_t(x);
}

inline
int DATETIME :: ToString( char * buffer ) const {
    tm dtime;
    time_t tmp = static_cast<time_t>(ctime);
    gmtime_r(&tmp, &dtime);

    const char * format = "%d-%02d-%02d %02d:%02d:%02d";

    return 1+sprintf(buffer, format,
        dtime.tm_year + TM_YEAR_OFFSET,
        dtime.tm_mon + 1,
        dtime.tm_mday,
        dtime.tm_hour,
        dtime.tm_min,
        dtime.tm_sec
    );
}

inline
void DATETIME :: FromJson( const Json::Value & src ) {
    this->FromString(src.asCString());
}

inline
void DATETIME :: ToJson( Json::Value & dest ) const {
    char buffer[24];
    this->ToString(buffer);
    dest = buffer;
}

<?  ob_start(); ?>

inline
void FromString( @type & date, const char * buffer ) {
    date.FromString( buffer );
}

inline
int ToString( const @type & date, char * buffer ) {
    return date.ToString(buffer);
}

inline
int64_t ClusterValue(const @type& x){
    // Cast to uint first so that the result is not sign extended
    uint32_t tmp = x.AsSeconds();
    return tmp;
}

inline
void FromJson( const Json::Value & src, @type & dest ) {
    dest.FromJson(src);
}

inline
void ToJson( const @type & src, Json::Value & dest ) {
    src.ToJson(dest);
}

// Hash function
<?  $functions[] = ['Hash', ['@type'], 'BASE::BIGINT', true, true ]; ?>
inline uint64_t Hash( const @type val ) {
    return val.AsSeconds();
}

<?  $functions[] = ['IsNull', ['@type'], 'BASE::BOOL', true, true ]; ?>
inline
bool IsNull( const @type & d ) {
    return d.IsNull();
}

#ifdef _HAS_STD_HASH
<?  $system_headers[] = 'functional'; ?>
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

<?  $globalContent .= ob_get_clean(); ?>

<?
    return [
        'kind'              => 'TYPE',
        'complex'           => false,
        'system_headers'    => $system_headers,
        'user_headers'      => [ 'Config.h' ],
        'binary_operators'  => $bin_operators,
        'global_content'    => $globalContent,
        'constructors'      => $constructors,
        'methods'           => $methods,
        'functions'         => $functions,
        'libraries'         => $libraries,
        'properties'        => [ 'clusterable' ],
        'describe_json'     => DescribeJson('datetime', DescribeJsonStatic(['format' => 'YYYY-MM-DD HH:mm:ss'])),
        'extras'            => [ 'size.bytes' => 4 ],
    ];
}

// DATETIME + INT = DATETIME
declareOperator( '+', [ 'base::DATETIME', 'base::INT' ],
    function($args, $targs = []) {
        $dateType = lookupType('base::DATETIME');
        $intType = lookupType('base::INT');

        return [
            'kind'      => 'OPERATOR',
            'input'     => [ $dateType, $intType ],
            'result'    => $dateType,
            'deterministic' => true,
        ];
    }
);

// DATETIME - INT = DATETIME
declareOperator( '-', [ 'base::DATETIME', 'base::INT' ],
    function($args, $targs = []) {
        $dateType = lookupType('base::DATETIME');
        $intType = lookupType('base::INT');

        return [
            'kind'      => 'OPERATOR',
            'input'     => [ $dateType, $intType ],
            'result'    => $dateType,
            'deterministic' => true,
        ];
    }
);

// DATETIME - DATETIME = INT
declareOperator( '-', [ 'base::DATETIME', 'base::DATETIME' ],
    function($args, $targs = []) {
        $dateType = lookupType('base::DATETIME');
        $intType = lookupType('base::INT');

        return [
            'kind'      => 'OPERATOR',
            'input'     => [ $dateType, $dateType ],
            'result'    => $intType,
            'deterministic' => true,
        ];
    }
);

foreach( [ '==', '!=', '<', '>', '<=', '>=' ] as $op ) {
    declareOperator( $op, [ 'base::DATETIME', 'base::INT' ],
        function($args, $targs = []) {
            $dateType = lookupType('base::DATETIME');
            $intType = lookupType('base::INT');
            $boolType = lookupType('base::BOOL');

            return [
                'kind'      => 'OPERATOR',
                'input'     => [ $dateType, $intType ],
                'result'    => $boolType,
                'deterministic' => true,
            ];
        }
    );
}
?>
