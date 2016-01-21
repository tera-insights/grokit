<?
// Copyright 2014, Tera Insights LLC. All Rights Reserved.

function TIME() {
    $methods = [];
    $constructors = [];
    $functions = [];
    $bin_operators = [];
    $libraries = [];

    $system_headers = [ 'cinttypes', 'limits', 'string', 'stdio.h' ];

    $system_headers[] = 'boost/date_time/posix_time/posix_time.hpp';
    $libraries[] = 'boost_date_time';

    $globalContent = "";
?>

class TIME {
private:
    int32_t nMillis;

    static constexpr int32_t LEAP_START = 86399000;
    static constexpr int32_t MAX_TIME = 863999999;
    static constexpr int32_t INVALID_TIME = std::numeric_limits<int32_t>::min();

    static constexpr int32_t SECONDS_TO_MILLIS = 1000;
    static constexpr int32_t MINUTES_TO_MILLIS = SECONDS_TO_MILLIS * 60;
    static constexpr int32_t HOURS_TO_MILLIS = MINUTES_TO_MILLIS * 60;

    static constexpr int32_t MILLIS_PER_SECOND = 1000;
    static constexpr int32_t SECONDS_PER_MINUTE = 60;
    static constexpr int32_t MINUTES_PER_HOUR = 60;

    static constexpr int32_t MillisFromFacets(const int32_t h, const int32_t m, const int32_t s, const int32_t ms);

public:
<?  $constructors[] = [[], true];  ?>
    constexpr TIME(void);

<?  $constructors[] = [['BASE::INT'], true];  ?>
    constexpr TIME( const int32_t _millis );

<?  $constructors[] = [['BASE::NULL'], true];  ?>
    constexpr TIME( const GrokitNull & null );

<?  $constructors[] = [['BASE::INT', 'BASE::INT', 'BASE::INT', 'BASE::INT'], true];  ?>
    constexpr TIME(
        const int32_t hours,
        const int32_t minutes,
        const int32_t seconds,
        const int32_t millis
    );


<?  $methods[] = [ 'hour', [], 'BASE::BYTE', true ];  ?>
    // Get the hours portion of the time
    constexpr int8_t hour(void) const;
    constexpr int8_t hours(void) const;
<?  $methods[] = [ 'minute', [], 'BASE::BYTE', true ];  ?>
    // Get the minutes portion of the time
    constexpr int8_t minute(void) const;
    constexpr int8_t minutes(void) const;
<?  $methods[] = [ 'second', [], 'BASE::BYTE', true ];  ?>
    // Get the seconds portion of the time
    constexpr int8_t second(void) const;
    constexpr int8_t seconds(void) const;
<?  $methods[] = [ 'milli', [], 'BASE::SMALLINT', true ];  ?>
    // Get the milliseconds portion of the time
    constexpr int16_t milli(void) const;
    constexpr int16_t millis(void) const;

<?  $methods[] = ['as_hours', [], 'BASE::INT', true ];  ?>
    // Get the time in hours since the beginning of the day
    constexpr int32_t as_hours(void) const;
<?  $methods[] = ['as_minutes', [], 'BASE::INT', true ];  ?>
    // Get the time in minutes since the beginning of the day
    constexpr int32_t as_minutes(void) const;
<?  $methods[] = ['as_seconds', [], 'BASE::INT', true]; ?>
    // Get the time in seconds since the beginning of the day
    constexpr int32_t as_seconds(void) const;
<?  $methods[] = ['as_millis', [], 'BASE::INT', true]; ?>
    // Get the time in milliseconds since the beginning of the day
    constexpr int32_t as_millis(void) const;


    constexpr bool is_valid(void) const;
    constexpr bool is_null(void) const;

    // Comparison operators
<?  $bin_operators[] = '==';  ?>
    constexpr bool operator ==( const TIME & o ) const;
<?  $bin_operators[] = '!=';  ?>
    constexpr bool operator !=( const TIME & o ) const;
<?  $bin_operators[] = '>';  ?>
    constexpr bool operator >( const TIME & o ) const;
<?  $bin_operators[] = '<';  ?>
    constexpr bool operator <( const TIME & o ) const; //>
<?  $bin_operators[] = '>=';  ?>
    constexpr bool operator >=( const TIME & o ) const;
<?  $bin_operators[] = '<=';  ?>
    constexpr bool operator <=( const TIME & o ) const; //>

    // Addition and subtraction of time intervals
<?  $bin_operators[] = '+';  ?>
    constexpr TIME operator +( const TIME & o ) const;
<?  $bin_operators[] = '-';  ?>
    constexpr TIME operator -( const TIME & o ) const;

    void FromString( const char * str );
    int ToString( char * buffer ) const;

    void FromJson( const Json::Value & );
    void ToJson( Json::Value & ) const;
};

inline
constexpr int32_t TIME :: MillisFromFacets(const int32_t h, const int32_t m, const int32_t s, const int32_t ms) {
    return (h * HOURS_TO_MILLIS) + (m * MINUTES_TO_MILLIS) + (s * SECONDS_TO_MILLIS) + ms;
}

inline
constexpr TIME :: TIME(void) : nMillis(INVALID_TIME)
{ }

inline
constexpr TIME :: TIME( const int32_t _millis ) : nMillis(_millis)
{ }

inline
constexpr TIME :: TIME( const int32_t h, const int32_t m, const int32_t s, const int32_t ms ) :
    nMillis(MillisFromFacets(h, m, s, ms))
{ }

inline
constexpr int32_t TIME :: as_hours(void) const {
    return nMillis / HOURS_TO_MILLIS;
}

inline
constexpr int8_t TIME :: hour(void) const {
    return as_hours();
}

inline
constexpr int8_t TIME :: hours(void) const {
    return hour();
}

inline
constexpr int32_t TIME :: as_minutes(void) const {
    return nMillis / MINUTES_TO_MILLIS;
}

inline
constexpr int8_t TIME :: minute(void) const {
    return as_minutes() % MINUTES_PER_HOUR;
}

inline
constexpr int8_t TIME :: minutes(void) const {
    return minute();
}

inline
constexpr int32_t TIME :: as_seconds(void) const {
    return nMillis / SECONDS_TO_MILLIS;
}

inline
constexpr int8_t TIME :: second(void) const {
    return (LEAP_START > nMillis) ? as_seconds() % SECONDS_PER_MINUTE : 60;
}

inline
constexpr int8_t TIME :: seconds(void) const {
    return second();
}

inline
constexpr int32_t TIME :: as_millis(void) const {
    return nMillis;
}

inline
constexpr int16_t TIME :: milli(void) const {
    return as_millis() % MILLIS_PER_SECOND;
}

inline
constexpr int16_t TIME :: millis(void) const {
    return milli();
}

inline
constexpr bool TIME :: is_valid(void) const {
    return nMillis >= 0 && MAX_TIME >= nMillis;
}

inline
constexpr bool TIME :: is_null(void) const {
    return nMillis == INVALID_TIME;
}

inline
constexpr bool TIME :: operator ==( const TIME & o ) const {
    return nMillis == o.nMillis;
}

inline
constexpr bool TIME :: operator !=( const TIME & o ) const {
    return nMillis != o.nMillis;
}

inline
constexpr bool TIME :: operator >( const TIME & o ) const {
    return nMillis > o.nMillis;
}

inline
constexpr bool TIME :: operator <( const TIME & o ) const { //>
    return nMillis < o.nMillis; //>
}

inline
constexpr bool TIME :: operator >=( const TIME & o ) const {
    return nMillis >= o.nMillis;
}

inline
constexpr bool TIME :: operator <=( const TIME & o ) const { //>
    return nMillis <= o.nMillis; //>
}

inline
constexpr TIME TIME :: operator +( const TIME & o ) const {
    return TIME(nMillis + o.nMillis);
}

inline
constexpr TIME TIME :: operator -( const TIME & o ) const {
    return TIME(nMillis - o.nMillis);
}

inline
void TIME :: FromString( const char * str ) {
    using namespace boost::posix_time;
    time_duration time = duration_from_string(std::string(str));
    nMillis = time.total_milliseconds();
}

inline
int TIME :: ToString( char * buffer) const {
    using namespace boost::posix_time;
    // Constructs boost::time_duration around nMillis. Boost automatically
    // truncates the string if there are no fractional seconds.
    std::string output = to_simple_string(milliseconds(nMillis)).substr(0, 12);
    // Only the first 12 characters are kept, i.e. HH:MM:SS.mmm
    strcpy(buffer, output.substr(0, 12).c_str());
    return 1 + output.length();
}

inline
void TIME :: FromJson( const Json::Value & src ) {
    this->FromString(src.asCString());
}

inline
void TIME :: ToJson( Json::Value & dest ) const {
    char buffer[9];
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
void FromJson( const Json::Value & src, @type & dest ) {
    dest.FromJson(src);
}

inline
void ToJson( const @type & src, Json::Value & dest ) {
    src.ToJson(dest);
}

// Hash function
<?  $functions[] = ['Hash', ['@type'], 'BASE::BIGINT', true, true ]; ?>
template<>
inline uint64_t Hash( const @type& val ) {
    return val.as_millis();
}

<?  $functions[] = ['IsNull', ['@type'], 'BASE::BOOL', true, true ]; ?>
inline
bool IsNull( const @type & val ) {
    return val.is_null();
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
        'system_headers'    => $system_headers,
        'libraries'         => $libraries,
        'user_headers'      => [ 'Config.h' ],
        'binary_operators'  => $bin_operators,
        'global_content'    => $globalContent,
        'constructors'      => $constructors,
        'methods'           => $methods,
        'functions'         => $functions,
        'describe_json'     => DescribeJson('time', DescribeJsonStatic(['format' => 'HH:mm:ss.SSS'])),
        'extras'            => [ 'size.bytes' => 4 ],
    ];
}
?>
