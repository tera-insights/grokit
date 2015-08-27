<?
// Copyright 2015 Tera Insights, LLC. All Rights Reserved.

// This type is used to store a set containing categorical information. Each
// level of the category is assigned an index and a bitset is used to record
// whether a given level is in the set or not.

// System headers:
//     cstdio - sprintf.

function CATEGORYSET($t_args) {
    // The dictionary of the associated factor.
    $dictionary = get_first_key($t_args, ['dictionary', 'dict', 0]);
    $values = array_values($dictionary);

    // The component types used.
    $category = lookupType('category', ['dict'   => $dictionary]);
    $bitset   = lookupType('bitset',   ['values' => $values]);

    $size = $bitset->get('size.bytes');

    // The name of the object type.
    $className = 'CategorySet' . $size;

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
    $describeJson    = DescribeJson('categoryset',
                                    DescribeJsonStatic(['levels' => $values]));

    $globalContent = '';
?>

class <?=$className?> {
 public:
  using Category = <?=$category?>;
  using BitSet = <?=$bitset?>;
  using StorageType = BitSet::StorageType;

 private:
  // The binary data corresponding to the encoded string.
  BitSet data;

 public:
<?  $constructors[] = [[], true]; ?>
  <?=$className?>();
<?  $constructors[] = [['BASE::NULL'], true]; ?>
  <?=$className?>(const GrokitNull& null);
<?  $constructors[] = [['BASE::STRING_LITERAL'], true]; ?>
  <?=$className?>(const char* str);
  <?=$className?>(const BitSet& data);
  <?=$className?>(const <?=$className?>& other);

<?  $methods[] = ['IsEmpty', [], 'BASE::BOOL', true];  ?>
  bool IsEmpty() const;

  <?=$className?>& operator =(const <?=$className?>& other) = default;
<?  $binaryOperators[] = '=='; ?>
  bool operator ==(const <?=$className?>& other) const;
<?  $binaryOperators[] = '!='; ?>
  bool operator !=(const <?=$className?>& other) const;
<?  $binaryOperators[] = '<'; ?>
  bool operator <(const <?=$className?>& other) const;
<?  $binaryOperators[] = '>'; ?>
  bool operator >(const <?=$className?>& other) const;
<?  $binaryOperators[] = '<='; ?>
  bool operator <=(const <?=$className?>& other) const;
<?  $binaryOperators[] = '>='; ?>
  bool operator >=(const <?=$className?>& other) const;

  void FromString(const char* str);
  int ToString(char* buffer) const;

  void FromJson(const Json::Value& src);
  void ToJson(Json::Value& dest) const;
};

inline <?=$className?>::<?=$className?>()
    : data() {
}

inline <?=$className?>::<?=$className?>(const GrokitNull& null)
    : data() {
}

inline <?=$className?>::<?=$className?>(const char* str) {
  this->FromString(str);
}

inline <?=$className?>::<?=$className?>(const <?=$className?>::BitSet& data)
    : data(data) {
}

inline <?=$className?>::<?=$className?>(const <?=$className?>& other)
    : data(other.data) {
}

inline bool <?=$className?>::IsEmpty() const {
  return data == 0;
}

inline bool <?=$className?>::operator ==(const <?=$className?>& other) const {
  return this->data == other.data;
}

inline bool <?=$className?>::operator !=(const <?=$className?>& other) const {
  return this->data != other.data;
}

inline bool <?=$className?>::operator <(const <?=$className?>& other) const {
  return this->data < other.data;
}

inline bool <?=$className?>::operator >(const <?=$className?>&other) const {
  return this->data > other.data;
}

inline bool <?=$className?>::operator <=(const <?=$className?>& other) const {
  return this->data <= other.data;
}

inline bool <?=$className?>::operator >=(const <?=$className?>& other) const {
  return this->data >= other.data;
}

inline void <?=$className?>::FromString(const char* str) {
  StorageType mask;
  char* storage;
  char* copy = strdup(str);
  char* token = strtok_r(copy, " ", &storage);
  while (token != NULL) {
    Category level (token);
    WARNINGIF(level.Invalid(), "Invalid token: %s", token);
    mask |= 1 << level;
    token = strtok_r(NULL, " ", &storage);
  }
  data = mask;
  free(copy);
}

inline int <?=$className?>::ToString(char* buffer) const {
  char* start = buffer;
<?  foreach ($dictionary as $name) { ?>
  if (data.<?=$name?>())
    buffer += sprintf(buffer, "%s ", "<?=$name?>");
<?  }?>
  if (start == buffer) {
    buffer[0] = '\0';
    return 1;
  } else {
    buffer[-1] = '\0';
    return buffer - start;
  }
}

inline void <?=$className?>::FromJson(const Json::Value& src) {
  this->FromString(src.asCString());
}

inline void <?=$className?>::ToJson(Json::Value& dest) const {
  char* buffer;
  this->ToString(buffer);
  dest = buffer;
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
