<?

function FACTOR( array $t_args ) {
    $rawDict = get_first_key( $t_args, ['dictionary', 'dict', 0] );

    // Double the quotes so that we escape them in SQLite, and add backslashes
    // to them so that we escape them in C++.
    $dict = addcslashes(\grokit\doubleChars($rawDict, '"'), '"\\');

    $cardinality = \grokit\dictionarySize($rawDict);

    $storageBytes = get_first_key_default( $t_args, [ 'bytes', 1 ], 2);
    $cardBytes = $cardinality > 0 ? intval(ceil(log($cardinality, 256))) : 1;
    $storageBytes = $cardBytes > $storageBytes ? $cardBytes : $storageBytes;

    switch($storageBytes) {
    case 1:
        $columnType = 'base::uint';
        $storageType = 'uint8_t';
        break;
    case 2:
        $columnType = 'base::uint';
        $storageType = 'uint16_t';
        break;
    case 4:
        $columnType = 'base::uint';
        $storageType = 'uint32_t';
        break;
    case 8:
        $columnType = 'base::bigint';
        $storageType = 'uint64_t';
        break;
    default:
        grokit_error('Unsupported # of bytes (' . $storageBytes . ') given for FACTOR, only 1, 2, 4, and 8 supported.');
    }

    $columnType = lookupType($columnType);

    $className = generate_name('FACTOR_' . ensure_identifier($dict));

    $stringType = lookupType('base::STRING');

    $globalContent = '';

    $methods = [];
    $constructors = [];
    $functions = [];
?>

class <?=$className?> {
public:
    typedef <?=$storageType?> StorageType;

    static const        char *          DictionaryName      __attribute__((weak));
    static const        StorageType     InvalidID           __attribute__((weak));
    static const        StorageType     MaxID               __attribute__((weak));
    static const        Dictionary &    globalDictionary    __attribute__((weak));

public:
    /* ----- Members ----- */

    // The ID of this Factor;
    StorageType myID;

    /* ----- Constructors / Destructors ----- */

    // Default constructor
    <?=$className?>( void );

    // Constructor from null (same as default)
    <?=$className?>( const GrokitNull & );

    // Constructor from C strings / string literals
<?  $constructors[] = [ [ 'base::STRING_LITERAL' ], true]; ?>
    <?=$className?>( const char * );

    // Constructor from Grokit STRING type.
<?  $constructors[] = [ ['base::STRING'], true]; ?>
    <?=$className?>( const <?=$stringType?> & );

    // Constructor from storage type
<?  $constructors[] = [ [$columnType], true]; ?>
    <?=$className?>( const StorageType );

    // Copy constructor and copy assignment
    // These can both be default
    <?=$className?>( const <?=$className?> & ) = default;
    <?=$className?> & operator =( const <?=$className?> & ) = default;

    // Destructor
    ~<?=$className?>() { }

    /* ----- Methods ----- */

    // Standard FromString method
    void FromString( const char * );

    // FromString method used when building the dictionaries.
    void FromString( const char *, Dictionary & );

    // Looks up the factor in the global dictionary and returns the string
<?  $methods[] = [ 'ToString', [], 'base::STRING_LITERAL', true ] ?>
    const char * ToString( void ) const;

    // Returns the ID of the Factor.
<?  $methods[] = [ 'GetID', [], $columnType, true ] ?>
    StorageType GetID( void ) const;

    // Returns whether or not the Factor is valid.
<?  $methods[] = [ 'Valid', [], 'base::bool', true ];    ?>
    bool Valid( void ) const;
<?  $methods[] = [ 'Invalid', [], 'base::bool', true ];    ?>
    bool Invalid( void ) const;

    // Translate the content
    void Translate( const Dictionary::TranslationTable& );

    void toJson( Json::Value & dest ) const;
    void fromJson( const Json::Value & src );

    /* ----- Operators ----- */

    // The dictionary keeps track of what the sorted order of the strings is.
    // These methods are based on the lexicographical ordering of the strings
    // the factors represent
    bool operator ==( const <?=$className?> & ) const;
    bool operator !=( const <?=$className?> & ) const;
    bool operator <( const <?=$className?> & ) const;
    bool operator <=( const <?=$className?> & ) const;
    bool operator >( const <?=$className?> & ) const;
    bool operator >=( const <?=$className?> & ) const;

    // Implicit conversion to storage type
    operator StorageType () const;
};

// Static member initialization
const <?=$className?>::StorageType <?=$className?>::InvalidID = std::numeric_limits<StorageType>::max();
const <?=$className?>::StorageType <?=$className?>::MaxID = <?=$className?>::InvalidID - 1;
const char * <?=$className?>::DictionaryName = "<?=$dict?>";
const Dictionary & <?=$className?>::globalDictionary = Dictionary::GetDictionary(<?=$className?>::DictionaryName);

/* ----- Constructors ----- */

// Default constructor
inline
<?=$className?> :: <?=$className?>( void ):
    myID(InvalidID)
{}

inline
<?=$className?> :: <?=$className?>( const GrokitNull & nullval ):
    myID(InvalidID)
{ }

// Constructor from C strings / string literals
inline
<?=$className?> :: <?=$className?>( const char * str ) {
    FromString(str);
}

// Constructor from Grokit STRING type
inline
<?=$className?> :: <?=$className?>( const <?=$stringType?> & str ) {
    FromString(str.ToString());
}

// Constructor from storage type
inline
<?=$className?> :: <?=$className?>( const <?=$storageType?> id ):
    myID(id)
{ }

/* ----- Methods ----- */

inline
auto <?=$className?> :: GetID(void) const -> StorageType {
    return myID;
}

// Standard FromString method
inline
void <?=$className?> :: FromString( const char * str ) {
    // Global dictionary will return InvalidID if not found
    myID = globalDictionary.Lookup(str, InvalidID );
}

// FromString method used when building the dictionaries
inline
void <?=$className?> :: FromString( const char * str, Dictionary & localDict ) {
    // First check if we are in the local dictionary
    myID = localDict.Lookup(str, InvalidID );
    if( myID != InvalidID )
        return;

    // Next check if we are in the global dictionary
    myID = globalDictionary.Lookup(str, InvalidID );
    if( myID != InvalidID )
        return;

    // Add a new entry to the local dictionary.
    // The dictionary should throw an error if the new ID is greater than
    // MaxID.
    myID = localDict.Insert( str, MaxID );
}

// Looks up the factor in the global dictionary and returns the string
inline
const char * <?=$className?> :: ToString( void ) const {
    return globalDictionary.Dereference(myID);
}

// Determine whether or not the factor is valid
inline
bool <?=$className?> :: Valid( void ) const {
    return myID != InvalidID;
}

inline
bool <?=$className?> :: Invalid(void) const {
    return myID == InvalidID;
}

// Translate the content
inline
void <?=$className?> :: Translate( const Dictionary::TranslationTable & tbl ) {
    auto it = tbl.find(myID);
    if( it != tbl.end() ) {
        myID = it->second;
    }
}


inline
void <?=$className?> :: toJson( Json::Value & dest ) const {
    dest = (Json::Int64) myID;
}

inline
void <?=$className?> :: fromJson( const Json::Value & src ) {
    myID = (StorageType) src.asInt64();
}

/* ----- Operators ----- */

inline
bool <?=$className?> :: operator ==( const <?=$className?> & o ) const {
    return myID == o.myID;
}

inline
bool <?=$className?> :: operator !=( const <?=$className?> & o ) const {
    return myID != o.myID;
}

inline
bool <?=$className?> :: operator <( const <?=$className?> & o ) const {
    return Valid() && o.Valid() && globalDictionary.Compare(myID, o.myID) < 0;
}

inline
bool <?=$className?> :: operator <=( const <?=$className?> & o ) const {
    return Valid() && o.Valid() && globalDictionary.Compare(myID, o.myID) <= 0;
}

inline
bool <?=$className?> :: operator >( const <?=$className?> & o ) const {
    return Valid() && o.Valid() && globalDictionary.Compare(myID, o.myID) > 0;
}

inline
bool <?=$className?> :: operator >=( const <?=$className?> & o ) const {
    return Valid() && o.Valid() && globalDictionary.Compare(myID, o.myID) >= 0;
}

// Implicit conversion to storage type
inline
<?=$className?> :: operator StorageType() const {
    return myID;
}

<?  ob_start(); // Global functions ?>

inline
void FromString( @type & f, const char * str ) {
    f.FromString(str);
}

inline
void FromString( @type & f, const char * str, Dictionary & localDict ) {
    f.FromString(str, localDict);
}

inline
int ToString( const @type & f, char * buffer ) {
    const char * str = f.ToString();
    strcpy(buffer, str);
    return strlen(buffer) + 1;
}

<?  $functions[] = [ 'Hash', [ '@type' ], 'base::BIGINT', true, true ] ?>
template<>
inline
uint64_t Hash( const @type & x ) {
    return x.GetID();
}

inline
void ToJson( const @type & src, Json::Value & dest ) {
    src.toJson(dest);
}

inline
void FromJson( const Json::Value & src, @type & dest ) {
    dest.fromJson(src);
}

<?  $functions[] = [ 'IsNull', ['@type'], 'BASE::BOOL', true, true]; ?>
inline
bool IsNull( const @type f ) {
    return f.Invalid();
}

<?  $globalContent .= ob_get_clean(); ?>

<?
    // Function to get the dictionary at runtime.
    $describeInfoJson = function($var, $myType) {
?>
    <?=$var?>["levels"] = Json::Value(Json::arrayValue);
    for( auto it = <?=$myType?>::globalDictionary.cbegin(); it != <?=$myType?>::globalDictionary.cend(); it++ ) {
        <?=$var?>["levels"][it->first] = it->second;
    }
<?
    };

    return [
        'kind'              => 'TYPE',
        'name'              => $className,
        'dictionary'        => $dict,
        'system_headers'    => [ 'limits', 'cstring', 'cinttypes' ],
        'user_headers'      => [ 'Dictionary.h', 'DictionaryManager.h', 'ColumnIteratorDict.h' ],
        'properties'        => [ 'categorical' ],
        'extras'            => [ 'cardinality' => $cardinality, 'size.bytes' => $storageBytes ],
        'binary_operators'  => [ '==', '!=', '<', '>', '<=', '>=' ],
        'global_content'    => $globalContent,
        'complex'           => 'ColumnIteratorDict< @type >',
        'methods'           => $methods,
        'constructors'      => $constructors,
        'functions'         => $functions,
        'describe_json'     => DescribeJson('factor', $describeInfoJson),
    ];

} // end function FACTOR

?>
