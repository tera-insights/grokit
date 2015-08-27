<?
// Copyright 2015 Tera Insights, LLC. All Rights Reserved.

// System headers:
//     cstring - memcpy, strlen.
//     cstdio - sprintf.
// Library headers:
//     base64 - encoding and decoding base 64 strings.

function BASE64($t_args) {
    // The number of bytes encoded per string.
    $size = get_first_key($t_args, ['size', 0]);

    // The number of characters that should be used to encode each string.
    // Padding is acceptable but not necessarily, hence the acceptable number of
    // characters is an interval.
    $min = ceil($size * 4 / 3);
    $max = 4 * ceil($min / 4);

    $className = 'Base64_' . $size;

    $systemHeaders   = ['cstring', 'cstdio'];
    $userHeaders     = [];
    $libHeaders      = ['base64.h'];
    $libraries       = [];
    $constructors    = [];
    $methods         = [];
    $functions       = [];
    $binaryOperators = [];
    $unaryOperators  = [];
    $globalContent   = '';
    $complex         = false;
    $properties      = [];
    $extra           = ['size.bytes' => 40];
    $describeJson    = DescribeJson('base64',
                                    DescribeJsonStatic(['size' => $size]));

    $globalContent = '';
?>

class <?=$className?> {
 public:
  // The number of bytes encoded.
  static const constexpr int kSize = <?=$size?>;

  // The maximum number of characters that an input string is expected to have.
  static const constexpr int kMax = <?=$max?>;

  // The minimum number of characters that an input string is expected to have.
  static const constexpr int kMin = <?=$min?>;

 private:
  std::array<char, kSize> bytes;

 public:
<?  $constructors[] = [[], true]; ?>
  <?=$className?>();
<?  $constructors[] = [['BASE::NULL'], true]; ?>
  <?=$className?>(const GrokitNull& nullval);
<?  $constructors[] = [['BASE::STRING_LITERAL'], true]; ?>
  <?=$className?>(const char* str);

<?  $binaryOperators[] = '=='; ?>
  bool operator ==(const <?=$className?>& other) const;
<?  $binaryOperators[] = '!='; ?>
  bool operator !=(const <?=$className?>& other) const;

  void FromString(const char* str);
  int ToString(char* buffer) const;

  void FromJson(const Json::Value& src);
  void ToJson(Json::Value& src) const;
};

inline <?=$className?>::<?=$className?>() {
  bytes.fill(0);
}

<?=$className?>::<?=$className?>(const GrokitNull& nullval) {
  bytes.fill(0);
}

inline  <?=$className?>::<?=$className?>(const char* str) {
  this->FromString(str);
}

inline bool <?=$className?>::operator ==(const <?=$className?>& other) const {
  return std::memcmp(bytes.data(), other.bytes.data(), kSize);
}

inline bool <?=$className?>::operator !=(const <?=$className?>& other) const {
  return !(*this == other);
}

inline void <?=$className?>::FromString(const char* str) {
  size_t length = strlen(str);
  FATALIF(kMax < length || length < kMin,"Illegal base64-%d input: '%s'",
          kSize, str);
  std::string message = base64_decode(std::string(str));
  std::memcpy(bytes.data(), message.data(), message.size());
}

inline int <?=$className?>::ToString(char* buffer) const {
  std::string encoded = base64_encode((unsigned char*) bytes.data(), kSize);
  return 1 + sprintf(buffer, "%s", encoded.c_str());
}

inline void <?=$className?>::FromJson(const Json::Value& src) {
  this->FromString(src.asCString());
}

inline void <?=$className?>::ToJson(Json::Value& dest) const {
  dest = base64_encode((unsigned char*) bytes.data(), kSize);
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
        'name'             => $className,
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
