<?
// Copyright 2014 Tera Insights, LLC. All Rights Reserved.

function BITSET( array $t_args ) {
    grokit_assert(array_key_exists('values', $t_args), 'No values specified for bitset!');
    $values = $t_args['values'];
    $indicies = array_keys($values);

    $maxIndex = \max($indicies);
    $minIndex = \min($indicies);
    grokit_assert($maxIndex < 64, 'Highest index of bitset must be less than 64');
    grokit_assert($minIndex >= 0, 'Indicies of bitset must be >= 0');

    $mask = 0;

    foreach( $values as $index => $name ) {
        $firstChar = substr($name, 0, 1);
        $arr = str_split($name);
        $valid = array_reduce($arr, function($res, $item) {
            $res = $res && (ctype_alnum($item) || $item == '_');
            return $res;
        }, ctype_alpha($firstChar) || $firstChar == '_');

        grokit_assert($valid, "Invalid name ($name) given for index ($index) in bitset.");

        $mask = $mask | (1 << $index);
    }

    $nBits = floor(pow(2, ceil(log($maxIndex + 1,2))));
    $nBits = \max(8, $nBits);
    $nHex = $nBits / 4;

    $storageType = "uint{$nBits}_t";
    switch($nBits) {
    case 8:
        $methodIntType = 'base::BYTE';
        break;
    case 16:
        $methodIntType = 'base::SMALLINT';
        break;
    case 32:
        $methodIntType = 'base::INT';
        break;
    case 64:
        $methodIntType = 'base::BIGINT';
        break;
    default:
        grokit_error('BITSET requires invalid number of bits (' . $nBits . ')');
    }

    $className = generate_name('BITSET');

    $methods = [];
    $constructors = [];
    $functions = [];

    $globalContents = "";
?>

class <?=$className?> {
public:
    typedef <?=$storageType?> StorageType;

private:
    StorageType bits;

    static constexpr StorageType _MASK_ = 0x<?=sprintf("%0{$nHex}X", $mask)?>;

public:

    <?=$className?>(void);
<?  $constructors[] = [ [ $methodIntType ], true]; ?>
    <?=$className?>(const StorageType _bits);

    <?=$className?> & operator =( const StorageType _bits );

    /***** Comparison Opeators *****/
    bool operator ==( const <?=$className?> & o ) const;
    bool operator !=( const <?=$className?> & o ) const;
    bool operator <( const <?=$className?> & o ) const;
    bool operator >( const <?=$className?> & o ) const;
    bool operator <=( const <?=$className?> & o ) const;
    bool operator >=( const <?=$className?> & o ) const;

    /***** Conversion *****/
    void ToJson( Json::Value & dest ) const;
    void FromJson( const Json::Value & src );

    /***** Accessors *****/
<?  $methods[] = [ 'Bits', [ ], $methodIntType, true ]; ?>
    StorageType Bits(void) const;

<?  $methods[] = [ 'IsSet', ['base::BYTE' ], 'base::bool', true ];  ?>
    // Whether or not a bit is set by index
    bool IsSet(unsigned char index) const;

    // Accessors for each value
<?  foreach($values as $index => $name) { ?>
<?      $methods[] = [ $name, [], 'base::bool', true ]; ?>
    bool <?=$name?>(void) const;
<?  } // for each value ?>
};

inline
<?=$className?> :: <?=$className?>( void ) : bits(0) { }

inline
<?=$className?> :: <?=$className?>( const StorageType _bits ) : bits(_bits) { }

inline
<?=$className?> & <?=$className?> :: operator = (const StorageType _bits) {
    bits = _bits;
    return *this;
}

inline
bool <?=$className?> :: operator == (const <?=$className?> & o ) const {
    return bits == o.bits;
}

inline
bool <?=$className?> :: operator != (const <?=$className?> & o ) const {
    return bits != o.bits;
}

inline
bool <?=$className?> :: operator < (const <?=$className?> & o ) const {
    return (bits == (bits & o.bits)) && (bits != o.bits);
}

inline
bool <?=$className?> :: operator > (const <?=$className?> & o ) const {
    return (bits == (bits | o.bits)) && (bits != o.bits);
}

inline
bool <?=$className?> :: operator <= (const <?=$className?> & o ) const {
    return bits == (bits & o.bits);
}

inline
bool <?=$className?> :: operator >= (const <?=$className?> & o ) const {
    return bits == (bits | o.bits);
}

inline
auto <?=$className?> :: Bits( void ) const -> StorageType {
    return bits;
}

inline
bool <?=$className?>::IsSet(unsigned char index) const {
    StorageType mask = ((StorageType) 1) << index; //>
    return bits & mask;
}

inline
void <?=$className?> :: ToJson( Json::Value & dest ) const {
    dest = (Json::Int64) bits;
}

inline
void <?=$className?> :: FromJson( const Json::Value & src ) {
    bits = (StorageType) src.asInt64();
}

<?  foreach($values as $index => $name ) { ?>
inline
bool <?=$className?>::<?=$name?>(void) const {
    return bits & 0x<?=sprintf("%X", (1 << $index))?>;
}

<?  } // for each value ?>

<?  ob_start(); ?>

<?  $functions[] = [ 'Hash', [ '@type' ], 'base::BIGINT', true, true ]; ?>
template<>
inline
uint64_t Hash(const @type & thing) {
    return thing.Bits();
}

inline
void FromString( @type & c, const char * str ) {
    c = atol(str);
}

inline
int ToString( const @type & c, char * buffer ) {
<?  $format = $nBits < 16 ? 'hh' : ($nBits < 32 ? 'h' : ($nBits < 64 ? '' : 'l' )) ?>
    sprintf(buffer, "%<?=$format?>d", c.Bits());
    return strlen(buffer) + 1;
}

inline
void ToJson( const @type & src, Json::Value & dest ) {
    src.ToJson(dest);
}

inline
void FromJson( const Json::Value & src, @type & dest ) {
    dest.FromJson(src);
}

<?  $globalContents .= ob_get_clean(); ?>

<?
    return [
        'kind'              => 'TYPE',
        'name'              => $className,
        'binary_operators'  => [ '==', '!=', '>', '<', '>=', '<=' ],
        'system_headers'    => [ 'cinttypes' ],
        'global_content'    => $globalContents,
        'complex'           => false,
        'methods'           => $methods,
        'constructors'      => $constructors,
        'functions'         => $functions,
        'describe_json'     => DescribeJson('integer'),
        'extras'            => [ 'size.bytes' => $nBits / 8 ],
    ];
}

?>
