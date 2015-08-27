<?
// Copyright 2015 Tera Insights, LLC. All Rights Reserved.

function POSIXTIME() {
    $systemHeaders   = ['cstdio'];
    $userHeaders     = ['Config.h'];
    $libHeaders      = [];
    $libraries       = [];
    $constructors    = [];
    $methods         = [];
    $functions       = [];
    $binaryOperators = [];
    $unaryOperators  = [];
    $globalContent   = '';
    $complex         = false;
    $properties      = ['clusterable'];
    $extra           = ['size.bytes' => 8];
    $describeJson    = DescribeJson('posixtime');

    $systemHeaders[] = 'boost/date_time/posix_time/posix_time.hpp';
    $systemHeaders[] = 'boost/date_time/gregorian/gregorian.hpp';
    $systemHeaders[] = 'boost/date_time/dst_rules.hpp';
    $systemHeaders[] = 'boost/date_time/local_time_adjustor.hpp';
    $systemHeaders[] = 'boost/date_time/c_local_time_adjustor.hpp';

    $libraries[] = 'boost_date_time';

    $globalContent = '';
?>
using namespace boost::posix_time;
using namespace boost::gregorian;

class POSIXTIME {
 private:
  boost::posix_time::ptime time;

 public:
<?  $constructors[] = [[], true]; ?>
  POSIXTIME();
<?  $constructors[] = [['BASE::Null'], true]; ?>
  POSIXTIME(const GrokitNull& null);
<?  $constructors[] = [['BASE::INT'], true]; ?>
  POSIXTIME(int seconds);
<?  $constructors[] = [['BASE::BIGINT'], true]; ?>
  POSIXTIME(long useconds);
  POSIXTIME(const boost::posix_time::ptime& time);

<?  $binaryOperators[] = '=='; ?>
  bool operator ==(const POSIXTIME& other) const;
<?  $binaryOperators[] = '!='; ?>
  bool operator !=(const POSIXTIME& other) const;
<?  $binaryOperators[] = '<'; ?>
  bool operator <(const POSIXTIME& other) const;
<?  $binaryOperators[] = '>'; ?>
  bool operator >(const POSIXTIME& other) const;
<?  $binaryOperators[] = '<='; ?>
  bool operator <=(const POSIXTIME& other) const;
<?  $binaryOperators[] = '>='; ?>
  bool operator >=(const POSIXTIME& other) const;

  void FromString(const char* buffer);
  int ToString(char* buffer) const;

  void FromJson(const Json::Value& src);
  void ToJson(Json::Value& src) const;
};

inline POSIXTIME::POSIXTIME()
    : time(ptime(not_a_date_time)) {
}

inline POSIXTIME::POSIXTIME(const GrokitNull& null)
    : time(ptime(not_a_date_time)) {
}

inline POSIXTIME::POSIXTIME(int seconds)
    : time(from_time_t(seconds)) {
}

inline POSIXTIME::POSIXTIME(long useconds)
    : time(ptime(date(1970, 1, 1), microseconds(useconds))) {
}

inline POSIXTIME::POSIXTIME(const boost::posix_time::ptime& time)
    : time(time) {
}

inline bool POSIXTIME::operator ==(const POSIXTIME& other) const {
  return this->time == other.time;
}

inline  bool POSIXTIME::operator !=(const POSIXTIME& other) const {
  return this->time != other.time;
}

inline bool POSIXTIME::operator <(const POSIXTIME& other) const {
  return this->time < other.time;
}

inline bool POSIXTIME::operator >(const POSIXTIME&other) const {
  return this->time > other.time;
}

inline bool POSIXTIME::operator <=(const POSIXTIME& other) const {
  return this->time <= other.time;
}

inline bool POSIXTIME::operator >=(const POSIXTIME& other) const {
  return this->time >= other.time;
}

inline void POSIXTIME::FromString(const char* str) {
  using namespace boost::posix_time;
  using namespace boost::gregorian;
  time = ptime(date(1970, 1, 1), milliseconds(atol(str)));
}

inline int POSIXTIME::ToString(char* buffer) const {
  using namespace boost::posix_time;
  using namespace boost::gregorian;
  auto diff = time - ptime(date(1970, 1, 1));
  return 1 + sprintf(buffer, "%li", diff.total_milliseconds());
}

inline void POSIXTIME::FromJson(const Json::Value& src) {
  using namespace boost::posix_time;
  using namespace boost::gregorian;
  time = ptime(date(1970, 1, 1), milliseconds(src.asInt64()));
}

inline void POSIXTIME::ToJson(Json::Value& dest) const {
  using namespace boost::posix_time;
  using namespace boost::gregorian;
  auto diff = time - ptime(date(1970, 1, 1));
  dest = (Json::Int64) diff.total_milliseconds();
}

<?  ob_start(); ?>

inline void FromString(@type& data, const char* buffer) {
  data.FromString(buffer);
}

inline int ToString(const @type& data, char* buffer) {
  return data.ToString(buffer);
}

inline void FromJson(const Json::Value& src, @type& dest) {
  dest.FromJson(src);
}

inline void ToJson(const @type& src, Json::Value& dest) {
  src.ToJson(dest);
}

<?  $globalContent .= ob_get_clean(); ?>

<?
    return [
        'kind'             => 'TYPE',
        'complex'          => $complex,
        'system_headers'   => $systemHeaders,
        'user_headers'     => $userHeaders,
        'lib_headers'      => $libHeaders,
        'libraries'        => $libraries,
        'binary_operators' => $binaryOperators,
        'unary_operators'  => $unaryOperators,
        'global_content'   => $globalContent,
        'constructors'     => $constructors,
        'methods'          => $methods,
        'functions'        => $functions,
        'libraries'        => $libraries,
        'properties'       => $properties,
        'extras'           => $extra,
        'describe_json'    => $describeJson,
    ];
}
?>
