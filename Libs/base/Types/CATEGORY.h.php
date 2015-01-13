<?

function CATEGORY( array $t_args ) {
    if( array_key_exists('dict', $t_args)) {
        $values = $t_args['dict'];
        $maxID = 0;
        foreach( $values as $id => $val ) {
            $maxID = \max($id, $maxID);
        }
    } else {
        $old_vals = get_first_key( $t_args, [ 'values', 0 ] );
        $startAt = get_first_key_default( $t_args, [ 'start.at' ], 0 );

        $values = [];
        $maxID = $startAt;
        foreach( $old_vals as $ind => $val) {
            $values[$maxID++] = $val;
        }
    }

    $cardinality = \count($values);

    // Add 1 to the cardinality for the invalid id
    $storageTypeBits = ceil(log($maxID + 1, 2));
    if( $storageTypeBits > 64 ) {
        // This should never happen. PHP would explode processing 2^64 values.
        grokit_error("Unable to store $cardinality values within 64 bits.");
    }
    else if( $storageTypeBits > 32 ) {
        $storageType = 'uint64_t';
        $storageBytes = 8;
    }
    else if( $storageTypeBits > 16 ) {
        $storageType = 'uint32_t';
        $storageBytes = 4;
    }
    else if( $storageTypeBits > 8 ) {
        $storageType = 'uint16_t';
        $storageBytes = 2;
    }
    else {
        $storageType = 'uint8_t';
        $storageBytes = 1;
    }

    $className = generate_name('CATEGORY');

    $stringType = lookupType('base::STRING');

    $methods = [];
    $constructors = [];
    $functions = [];
?>

class <?=$className?> {
public:
    typedef <?=$storageType?> StorageType;
    typedef std::unordered_map<StorageType, std::string> IDToNameMap;
    typedef std::unordered_map<std::string, StorageType> NameToIDMap;

    static const StorageType InvalidID __attribute__((weak));

private:
    static const IDToNameMap idToName __attribute__((weak));
    static const NameToIDMap nameToID __attribute__((weak));

    // The ID of this categorical variable
    StorageType myID;

public:

    /* ----- Constructors / Destructor ----- */
    <?=$className?>( void );

<?  $constructors[] = [ [ 'base::STRING_LITERAL' ], true]; ?>
    <?=$className?>( const char * );
<?  $constructors[] = [ ['base::STRING'], true]; ?>
    <?=$className?>( const <?=$stringType?> & );
    <?=$className?>( const <?=$storageType?> );
    <?=$className?>( const <?=$className?> & );
<?  $constructors[] = [['BASE::NULL'], true]; ?>
    <?=$className?>( const GrokitNull & );

    <?=$className?> & operator =( const <?=$className?> & ) = default;

    ~<?=$className?>(void) {}

    /* ----- Methods ----- */
    void FromString( const char * );

<?  $methods[] = [ 'ToString', [], 'base::STRING_LITERAL', true ] ?>
    const char * ToString( void ) const;

    StorageType GetID( void ) const;
    void SetID( StorageType id );

    // Determines whether or not the category is valid.
<?  $methods[] = [ 'Invalid', [], 'base::bool', true ];    ?>
    bool Invalid(void) const;
<?  $methods[] = [ 'Valid', [], 'base::bool', true ];    ?>
    bool Valid(void) const;

    /* ----- Operators ----- */
    bool operator ==( const <?=$className?> & ) const;
    bool operator !=( const <?=$className?> & ) const;
    bool operator <( const <?=$className?> & ) const;
    bool operator <=( const <?=$className?> & ) const;
    bool operator >( const <?=$className?> & ) const;
    bool operator >=( const <?=$className?> & ) const;

    // Implicit conversion to storage type
    operator <?=$storageType?>() const;

    // To/From Json
    void toJson( Json::Value & dest ) const;
    void fromJson( const Json::Value & src );
};

/* ----- Constructors ----- */

inline
<?=$className?> :: <?=$className?>( void ) :
    myID(InvalidID)
{ }

inline
<?=$className?> :: <?=$className?>( const char * str ) {
    FromString(str);
}

inline
<?=$className?> :: <?=$className?>( const <?=$stringType?> & str ) {
    FromString(str.ToString());
}

inline
<?=$className?> :: <?=$className?>( const <?=$storageType?> val ) :
    myID(val)
{ }

inline
<?=$className?> :: <?=$className?>( const <?=$className?> & other ) : myID(other.myID)
{ }

inline
<?=$className?> :: <?=$className?>( const GrokitNull & nullval ) : myID(InvalidID)
{ }

/* ----- Methods ----- */

inline
void <?=$className?> :: FromString( const char * str ) {
    auto it = nameToID.find(str);
    if( it != nameToID.end() ) {
        myID = it->second;
    }
    else {
        myID = InvalidID;
    }
}

inline
const char * <?=$className?> :: ToString( void ) const {
    auto it = idToName.find(myID);
    if( it != idToName.end() ) {
        return it->second.c_str();
    }
    else {
        return "NULL";
    }
}

inline
auto <?=$className?> :: GetID( void ) const -> StorageType {
    return myID;
}

inline
void <?=$className?> :: SetID( StorageType id ) {
    myID = id;
}

inline
bool <?=$className?> :: Valid(void) const {
    return idToName.count(myID) > 0;
}

inline
bool <?=$className?> :: Invalid(void) const {
    return ! Valid();
}


/* ----- Operators ----- */
inline
bool <?=$className?> :: operator ==( const <?=$className?> & other ) const {
    return myID == other.myID;
}

inline
bool <?=$className?> :: operator !=( const <?=$className?> & other ) const {
    return myID != other.myID;
}

inline
bool <?=$className?> :: operator <( const <?=$className?> & other ) const {
    return myID < other.myID;
}

inline
bool <?=$className?> :: operator >( const <?=$className?> & other ) const {
    return myID > other.myID;
}

inline
bool <?=$className?> :: operator <=( const <?=$className?> & other ) const {
    return myID <= other.myID;
}

inline
bool <?=$className?> :: operator >=( const <?=$className?> & other ) const {
    return myID >= other.myID;
}

// To/From Json
inline
void <?=$className?> :: toJson( Json::Value & dest ) const {
    dest = (Json::Int64) myID;
}

inline
void <?=$className?> :: fromJson( const Json::Value & src ) {
    myID = (StorageType) src.asInt64();
}

inline
<?=$className?> :: operator <?=$storageType?> () const {
    return myID;
}

<?  ob_start(); ?>
<?  $functions[] = [ 'Hash', [ '@type' ], 'base::BIGINT', true, true ] ?>
inline
uint64_t Hash(const @type & thing) {
    return thing.GetID();
}

inline
void FromString( @type & c, const char * str ) {
    c.FromString(str);
}

inline
int ToString( const @type & c, char * buffer ) {
    const char * str = c.ToString();
    strcpy( buffer, str);
    int len = strlen(buffer);
    return len + 1;
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
bool IsNull( const @type c ) {
    return c.Invalid();
}
<?  $globalContents = ob_get_clean(); ?>

// Initialize static values
const <?=$className?>::IDToNameMap <?=$className?> :: idToName = { <?=array_template('{{key},"{val}"}', ',', $values)?> };
const <?=$className?>::NameToIDMap <?=$className?> :: nameToID = { <?=array_template('{"{val}",{key}}', ',', $values)?> };
const <?=$className?>::StorageType <?=$className?> :: InvalidID = std::numeric_limits<<?=$className?>::StorageType>::max();

<?
    return [
        'kind'             => 'TYPE',
        'name'             => $className,
        'properties'       => [ 'categorical' ],
        'extras'           => [ 'cardinality' => $cardinality, 'size.bytes' => $storageBytes ],
        'binary_operators' => [ '==', '!=', '<', '>', '<=', '>=' ],
        'system_headers'   => [ 'cinttypes', 'unordered_map', 'string', 'cstring', 'limits' ],
        'global_content'   => $globalContents,
        'complex'          => false,
        'methods'          => $methods,
        'constructors'     => $constructors,
        'functions'        => $functions,
        'describe_json' => DescribeJson('factor', DescribeJsonStatic([ 'levels' => $values ])),
        ];

} // end function CATEGORY

?>
