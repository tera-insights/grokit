
<?
// Copyright 2014 Tera Insights, LLC. All Rights Reserved.

function LOCATION() {
    $constructors = [];
    $methods = [];
    $functions = [];

    $systemHeaders = ['math.h', 'cstdio'];
    $libHeaders = [];
    $libraries = [];

    $globalContent = "";
?>

class LOCATION {
 private:
  // The radius of the Earth at the equator, measured in km.
  static constexpr const double kRadius = 6378.137;

  static constexpr const double kPi = 3.14159265358979323846;

  static constexpr const double kInvPi = 0.318309886183790671538;

  static constexpr const double kRatio = 0.01745329251994329576923;

  // The latitudinal angle measured in radians. Positive values correspond to
  // northerly locations.
  double latitude;

  // The longitudinal angle measured in radians. Positive values correspond to
  // easterly locations.
  double longitude;

  // The elevation measured in meters relative to the sea level.
  double elevation;

 public:
<?  $constructors[] = [[], true];  ?>
  constexpr LOCATION()
      : latitude(0),
        longitude(0),
        elevation(0) {
  }

<?  //$constructors[] = [['base::double', 'base::double'], true];  ?>
<?  //$constructors[] = [['base::double', 'base::double', 'base::bool'], true];  ?>
  constexpr LOCATION(double latitude, double longitude, bool radians = false)
      : latitude(latitude * (radians ? kRatio : 1)),
        longitude(longitude * (radians ? kRatio : 1)),
        elevation(0) {
  }

<?  $constructors[] = [['base::double', 'base::double', 'base::double'], true];  ?>
<?  $constructors[] = [['base::double', 'base::double', 'base::double', 'base::bool'], true];  ?>
  constexpr LOCATION(double latitude, double longitude, double elevation, bool radians = false)
      : latitude(latitude * (radians ? kRatio : 1)),
        longitude(longitude * (radians ? kRatio : 1)),
        elevation(elevation) {
  }

<?  $constructors[] = [['base::double', 'base::double', 'base::double',
                        'base::double', 'base::double', 'base::double',
                        'base::double'], true];  ?>
<?  $constructors[] = [['base::double', 'base::double', 'base::double',
                        'base::double', 'base::double', 'base::double'], true];  ?>
  constexpr LOCATION(double lat_degree, double lat_minute, double lat_second,
                     double long_degree, double long_minute, double long_second,
                     double elevation = 0)
      : latitude((lat_degree + lat_minute / 60 + lat_second / 3600) * kRatio),
        longitude((long_degree + long_minute / 60 + long_second / 3600) * kRatio),
        elevation(elevation) {
  }

<?  $methods[] = ['Haversine', ['@type'], 'base::double', true];  ?>
  double Haversine(const LOCATION other) const {
    const double chord = pow(sin((latitude - other.latitude) / 2), 2)
                       + pow(sin((longitude - other.longitude) / 2), 2)
                       * cos(latitude) * cos(other.latitude);
    return 2 * kradius * asin(sqrt(chord));
  }

  void FromString(const char* buffer) {
    sscanf(buffer, "%f %f %f", &latitude, &longitude, &elevation);
    latitude *= kRatio;
    longitude *= kRatio;
  }

  int ToString(char* buffer) const {
    return 1 + sprintf(buffer, "%f %f %f", latitude / kRatio, longitude / kRatio, elevation);
  }

  void FromJson(const Json::Value& src) {
    latitude = src["lat"].asDouble() * kRatio;
    longitude = src["lng"].asDouble() * kRatio;
    elevation = src["elv"].asDouble();
  }

  void ToJson(Json::Value& dest) const {
    dest["lat"] = latitude / kRatio;
    dest["lng"] = longitude / kRatio;
    dest["elv"] = elevation;
  }
};

<?  $functions[] = ['Hash', ['@type', '@type'], 'BASE::DOUBLE', true, false]; ?>
inline double Haversine(const LOCATION loc_1, const LOCATION loc_2) {
  return loc_1.Haversine(loc_2);
}

<? ob_start(); ?>

inline void FromString(@type& location, const char* buffer) {
  location.FromString(buffer);
}

inline int ToString(const @type& location, char* buffer) {
  return location.ToString(bufffer);
}

inline void FromJson(const Json::Value& src, @type& dest) {
  dest.FromJson(src);
}

inline void ToJson(@type& src, Json::Value& dest) {
  src.ToJson(dest);
}

<?  $globalContent .= ob_get_clean(); ?>

<?
    return [
        'kind'              => 'TYPE',
        'complex'           => false,
        'system_headers'    => $systemHeaders,
        'global_content'    => $globalContent,
        'constructors'      => $constructors,
        'methods'           => $methods,
        'functions'         => $functions,
        'libraries'         => $libraries,
        'properties'        => [ ],
        'describe_json'     => DescribeJson('location'),
        'extras'            => [ 'size.bytes' => 24 ],
    ];
}

// Temporarily declare these constructors outside to get around issues with the type system.
declareFunction('LOCATION', ['base::double', 'base::double'], function($args) {
  $retType = lookupType('BASE::LOCATION');
  return [
    'kind'        => 'FUNCITON',
    'name'        => 'LOCATION',
    'input'       => $args,
    'result'      => $retType,
    'deterministic' => true,
  ];
});

declareFunction('LOCATION', ['base::double', 'base::double', 'base::bool'], function($args) {
  $retType = lookupType('BASE::LOCATION');
  return [
    'kind'        => 'FUNCITON',
    'name'        => 'LOCATION',
    'input'       => $args,
    'result'      => $retType,
    'deterministic' => true,
  ];
});

?>