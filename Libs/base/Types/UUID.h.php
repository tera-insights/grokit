<?
// Copyright 2015 Tera Insights, LLC. All Rights Reserved.

function UUID() {
    $systemHeaders   = [];
    $userHeaders     = [];
    $libHeaders      = [];
    $libraries       = [];
    $constructors    = [];
    $methods         = [];
    $functions       = [];
    $binaryOperators = ['==', '!=', '>', '<', '>=', '<='];
    $unaryOperators  = [];
    $globalContent   = '';
    $complex         = false;
    $properties      = ['_primative_'];
    $extra           = ['size.bytes' => 16];
    $describeJson    = DescribeJson('uuid');

    $systemHeaders[] = 'boost/uuid/uuid.hpp';
    $systemHeaders[] = 'boost/uuid/uuid_generators.hpp';
    $systemHeaders[] = 'boost/uuid/string_generator.hpp';
    $systemHeaders[] = 'boost/lexical_cast.hpp';
    $systemHeaders[] = 'boost/uuid/uuid_io.hpp';

    $globalContent = '';
?>

using UUID = boost::uuids::uuid;

<?  $constructors[] = [['BASE::NULL'], true, 'UUID_Null']; ?>
inline UUID UUID_Null(const GrokitNull& null) {
  boost::uuids::nil_generator gen;
  return gen();
}

<?  ob_start(); ?>

inline void FromString(@type& uuid, const char* buffer) {
  boost::uuids::string_generator converter;
  uuid = converter(std::string(buffer));
}

inline int ToString(const @type& uuid, char* buffer) {
  const std::string tmp = boost::lexical_cast<std::string>(uuid);
  return 1 + sprintf(buffer, "%s", tmp.c_str());
}

inline void FromJson(const Json::Value& src, @type& dest) {
  FromString(dest, src.asCString());
}

inline void ToJson(const @type& src, Json::Value& dest) {
  dest = boost::lexical_cast<std::string>(src);
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
