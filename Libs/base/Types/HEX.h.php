<?
// Copyright 2015 Tera Insights, LLC. All Rights Reserved.

// This type is used to store arbitrary binary information encoded as a
// hexadecimal string, with two characters corresponding to one byte.

// System headers:
//     cstdio - sprintf.
// Library headers:
//     hex - conversion of bytes to hex characters.

function HEX($t_args) {
    // The number of bytes encoded per string.
    $size = get_first_key($t_args, ['size', 0]);

    $className = 'Hex' . $size;

    $systemHeaders   = ['cstdio', 'cstring'];
    $userHeaders     = [];
    $libHeaders      = [];
    $libraries       = [];
    $constructors    = [];
    $methods         = [];
    $functions       = [];
    $binaryOperators = [];
    $unaryOperators  = [];
    $globalContent   = '';
    $complex         = false;
    $properties      = [];
    $extra           = ['size.bytes' => $size];
    $describeJson    = DescribeJson('hex',
                                    DescribeJsonStatic(['size' => $size]));

    $globalContent = '';
?>

class <?=$className?> {
 public:
  // Lookup table per pair of hex digits.
  static constexpr const char lookup[256] __attribute__((weak)) = {
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
   -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
  };

  static constexpr const char to_char[16] __attribute__((weak)) = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
  };

  // The number of bytes encoded.
  static const constexpr size_t kSize = <?=$size?>;

  // The length of decoded strings.
  static const constexpr size_t kLength = 2 * kSize;

 private:
  // The binary data corresponding to the encoded string.
  std::array<char, kSize> bytes;

 public:
<?  $constructors[] = [[], true]; ?>
  <?=$className?>();
<?  $constructors[] = [['BASE::NULL'], true]; ?>
  <?=$className?>(const GrokitNull& nullval);
<?  $constructors[] = [['BASE::STRING_LITERAL'], true]; ?>
  <?=$className?>(const char* str);

<?  $methods[] = ['IsValid', [], 'BASE::BOOL', true];  ?>
  bool IsValid(void) const;
<?  $methods[] = ['IsNull', [], 'BASE::BOOL', true];  ?>
  bool IsNull(void) const;

<?  $binaryOperators[] = '=='; ?>
  bool operator ==(const <?=$className?>& other) const;
<?  $binaryOperators[] = '!='; ?>
  bool operator !=(const <?=$className?>& other) const;

  void FromString(const char* str);
  int ToString(char* buffer) const;

  void FromJson(const Json::Value& src);
  void ToJson(Json::Value& dest) const;
};

// Allocate storage for static variables
constexpr const char <?=$className?>::lookup[256];
constexpr const char <?=$className?>::to_char[16];

inline <?=$className?>::<?=$className?>() {
  bytes.fill(0);
}

inline <?=$className?>::<?=$className?>(const GrokitNull& nullval) {
  bytes.fill(0);
}

inline <?=$className?>::<?=$className?>(const char* str) {
  this->FromString(str);
}

inline bool <?=$className?>::operator ==(const <?=$className?>& other) const {
  return std::memcmp(bytes.data(), other.bytes.data(), kSize);
}

inline bool <?=$className?>::operator !=(const <?=$className?>& other) const {
  return !(*this == other);
}

inline void <?=$className?>::FromString(const char* str) {
  size_t len = strlen(str);
  FATALIF(len != kLength, "Incorrectly sized string, expected %zu, got %zu: %s",
    kLength, len, str);
  for (size_t i = 0; i < kSize; i++) {
    char a = lookup[str[2 * i]];
    char b = lookup[str[2 * i + 1]];
    FATALIF(a < 0 || b < 0, "Illegal hex character: %c%c",
            str[2 * i], str[2 * i + 1]);
    bytes[i] = (a << 4) | b;
  }
}

inline int <?=$className?>::ToString(char* buffer) const {
  for (size_t i = 0; i < kSize; i++) {
    char byte = bytes[i];
    buffer[2 * i] = to_char[(byte >> 4) & 0x0F];
    buffer[2 * i + 1] = to_char[byte & 0x0F];
  }
  buffer[kLength] = '\0';
  return 1 + kLength;
}

inline void <?=$className?>::FromJson(const Json::Value& src) {
  this->FromString(src.asCString());
}

inline void <?=$className?>::ToJson(Json::Value& dest) const {
  char buffer[kLength + 1];
  this->ToString(buffer);
  dest = buffer;
}

<?  ob_start(); ?>

inline void FromString(@type& data, const char* buffer) {
  data.FromString(buffer);;
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
