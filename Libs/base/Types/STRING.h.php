
<?
//  Copyright 2012 Christopher Dudley, Apache 2.0 License
//  Copyright 2013, Tera Insights. All rights reserved

/*
 * A basic string type, representing null-terminated strings.
 */


function STRING( array $t_args ){
    $debug = get_default($t_args, 'debug', 0);
    $deepCon = get_default($t_args, 'construct.deep', true);
    $deepAssign = get_default($t_args, 'assign.deep', false);

    // global content
    $gContent = "";

    $className = \count($t_args) > 0 ? generate_name("STRING") : "STRING";

    $constructors = [];
    $methods = [];
    $functions = [];
?>

class <?=$className?> {

    // the type used for the size
    typedef size_t SizeType;

    // The length of the string
    SizeType length;

    // The null-terminated string
    const char * str;

    // Whether or not the string is responsible for its own storage.
    bool localStorage;

    ///// Private Functions /////

    // Clears the value of the string, deallocating memory if necessary.
    void Clear();

public:

    // The maximum length for a string in the system.
    static const SizeType MaxObjectLength = 1024;
    static const SizeType HeaderLength = 1024;

    // Use the DELETE character as the NULL, since 0 is end of string.
    // We'll use 0xFF as the null character because it is not a valid
    // character in UTF-8
    static const char NULL_CHAR = 0xFF;

    // Statically allocate the null string.
    static const char NULL_STR[2] __attribute__((weak));

    // Empty string
    <?=$className?>();

    <?=$className?>( <?=$className?> && );

    // Construct a NULL string
<?  $constructors[] = [['BASE::NULL'], true]; ?>
    <?=$className?>( const GrokitNull & n );

    // Construct the object on top of the given null terminated string.
<?  $constructors[] = [['base::STRING_LITERAL'], true];  ?>
    <?=$className?>( const char * _str );

    // Create string from at most n characters in the given string
    <?=$className?>( const char *_str, size_t n );

    // Create string from STL string
    <?=$className?>( const std::string & str );

    // Make a copy of the other string (deep)
    <?=$className?>( const <?=$className?>& other );

    // Copy assigment operator (shallow)
    <?=$className?>& operator = ( const <?=$className?>& other );

    // Destructor
    ~<?=$className?>();

    ///// Serialization / Deserialization operations /////

    // Serialize myself to the buffer.
    char * Serialize( char * buffer ) const;
    void * Serialize( void * buffer ) const;

    // Deserialize myself from the buffer
    void Deserialize( char * buffer );

    // Return the size (in bytes) this object writes in Serialize and reads in Deserialize.
    int GetObjLength() const;
    int GetSize() const;

    ///// Utilities /////

    // Copy the data from the other string into this one (deep copy)
    void Copy( const <?=$className?>& other );

    void Copy( const char * str );

    ///// General Methods /////

    // Return the length of the string
<?  $methods[] = ['Length', [], 'base::INT', true]; ?>
    SizeType Length( void ) const;

<?  $methods[] = ['CharAt', [ 'base::INT' ], [ 'base::BYTE' ], true ]; ?>
    unsigned char CharAt( int i ) const;

<?  $methods[] = ['IsNull', [], [ 'BASE::BOOL' ], true]; ?>
    bool IsNull(void) const;

    // Return the null-terminated string the object represents
    const char * ToString( void ) const;

    void toJson( Json::Value & dest ) const;
    void fromJson( const Json::Value & src );

    // Operators

    // Access characters in the string.
    // Performs no bounds checking
    const char & operator [] ( SizeType index ) const;

    // Read a character at the specified index in the string.
    // Performs index sanity checking.
    const char & at( SizeType index ) const;

<?  if( $debug > 0 ) {    ?>
    // DEBUGGING
    std::string DebugString(void) const {
        std::ostringstream ostream;
        ostream << "Length: " << length << " local: " << localStorage << " str: " << std::string(str);
        return ostream.str();
//>
    }
<?  } // if debug > 0 ?>
};

const char <?=$className?> :: NULL_STR[2] = { <?=$className?>::NULL_CHAR, '\0' };

inline
<?=$className?> :: <?=$className?>() : length(0), str(""), localStorage(false) {
}

inline
<?=$className?> :: <?=$className?>( <?=$className?> && o ) :
    length(o.length),
    str(o.str),
    localStorage(o.localStorage)
{
    o.length = 0;
    o.str = "";
    o.localStorage = false;
}

inline
<?=$className?> :: <?=$className?>(const GrokitNull & n) : length(1), str(NULL_STR), localStorage(false) { }

inline
<?=$className?> :: <?=$className?>( const char * _str ) :
<?  if( $deepCon ) { ?>
    length(strlen(_str)), str(strdup( _str )), localStorage(true)
<?  } else { ?>
    length(strlen(_str)), str(_str), localStorage(false)
<?  } ?>
{
}

inline
<?=$className?> :: <?=$className?>( const char *_str, size_t n ) {
    char * buffer = (char *) malloc( sizeof(char) *  (n+1) );
    localStorage = true;
    strncpy(buffer, _str, n);
    buffer[n] = '\0';
    str = buffer;
    length = strlen(str);
}

inline
<?=$className?> :: <?=$className?>( const std::string & str ) :
    <?=$className?>(str.c_str())
{ }

inline
<?=$className?> :: <?=$className?>( const <?=$className?>& other ) : length(other.length), str(strdup(other.str)), localStorage(true) {
}

inline
<?=$className?>& <?=$className?> :: operator = ( const <?=$className?>& other ) {
    Clear();

    length = other.length;
<?  if( $deepAssign ) { ?>
    localStorage = true;
    str = strdup( other.str );
<?  } else { ?>
    localStorage = other.localStorage;
    str = localStorage ? strdup(other.str) : other.str;
<?  } ?>

    return *this;
}

inline
<?=$className?> :: ~<?=$className?>() {
    Clear();
}

inline
void <?=$className?> :: Clear() {
    if( localStorage ) {
        free((void *) str);
        str = "";
        length = 0;
        localStorage = false;
    }
}

inline
char * <?=$className?> :: Serialize( char * buffer ) const {
<?  if( $debug > 0 ) {    ?>
    std::cout << "Serializing -> " << DebugString() << std::endl;
<?  } // if debug > 0 ?>

    strcpy( buffer, str );

    return buffer;
}
//>

inline
void * <?=$className?> :: Serialize( void * buffer ) const {
    return (void *) Serialize( (char *) buffer );
}

inline
void <?=$className?> :: Deserialize( char * buffer ) {
    Clear();

    str = buffer;
    length = strlen( str );

<?  if( $debug > 0 ) {    ?>
    std::cout << "Deserialized -> " << DebugString() << std::endl;
<?  } // if debug > 0 ?>
}
//>

// Used for serialization
inline
int <?=$className?> :: GetObjLength() const {
    return length + 1;
}

// Used for serialization
inline
int <?=$className?> :: GetSize() const {
    return length + 1;
}

inline
void <?=$className?> :: Copy( const <?=$className?>& other ) {
    Clear();

    length = other.length;
    str = strdup( other.str );
    localStorage = true;
}

inline
void <?=$className?> :: Copy( const char * ostr ) {
    Clear();

    length = strlen(str);
    str = strdup(ostr);
    localStorage = true;
}

inline
<?=$className?>::SizeType <?=$className?> :: Length( void ) const {
    return length;
}

inline
unsigned char <?=$className?> :: CharAt( int i ) const {
    return str[i];
}

inline
bool <?=$className?> :: IsNull( void ) const {
    return length == 1 && str[0] == NULL_CHAR;
}

inline
const char * <?=$className?> :: ToString( void ) const {
    return str;
}

inline
void <?=$className?> :: toJson( Json::Value & dest ) const {
    dest = Json::Value(str);
}

inline
void <?=$className?> :: fromJson( const Json::Value & src ) {
    Clear();

    str = strdup(src.asCString());
    length = strlen(str);
    localStorage = true;
}

<? ob_start(); ?>

// Copy function
inline
void Copy( @type& to, const @type& from ) {
    to.Copy( from );
}

// ToString for Print
inline
int ToString( const @type & str, char* text ) {
<?  if( $debug > 0 ) {    ?>
    std::cout << "ToString -> " << str.DebugString() << std::endl;
<?  } // if debug > 0 ?>
    strcpy( text, str.ToString() );
    return str.Length() + 1;
}

// FromString for TextLoader
inline
void FromString( @type & str, const char * text ) {
    str = @type(text);
<?  if( $debug > 0 ) {    ?>
    std::cout << "FromString -> " << str.DebugString() << std::endl;
<?  } // if debug > 0 ?>
}

// >

inline
void ToJson( const @type & src, Json::Value & dest ) {
    src.toJson(dest);
}

inline
void FromJson( const Json::Value & src, @type & dest ) {
    dest.fromJson(src);
}

// ostream operator for easier debugging.
template<class CharT, class Traits = std::char_traits<CharT>>
std::basic_ostream<CharT, Traits>& operator << ( std::basic_ostream<CharT, Traits> & os, const @type s ) {
    return os << s.ToString(); //>>
}

<? $gContent .= ob_get_clean(); ?>

// Operators
inline
bool operator == (const <?=$className?>& str1, const <?=$className?>& str2) {
    return strcmp( str1.ToString(), str2.ToString() ) == 0;
}

inline
bool operator != (const <?=$className?>& str1, const <?=$className?>& str2) {
    return strcmp( str1.ToString(), str2.ToString() ) != 0;
}

inline
bool operator > (const <?=$className?>& str1, const <?=$className?>& str2) {
    return strcmp( str1.ToString(), str2.ToString() ) > 0;
}

inline
bool operator >= (const <?=$className?>& str1, const <?=$className?>& str2) {
    return strcmp( str1.ToString(), str2.ToString() ) >= 0;
}

inline
bool operator < (const <?=$className?>& str1, const <?=$className?>& str2) {
    return strcmp( str1.ToString(), str2.ToString() ) < 0;
}

inline
bool operator <= (const <?=$className?>& str1, const <?=$className?>& str2) {
    return strcmp( str1.ToString(), str2.ToString() ) <= 0;
}

inline
const char & <?=$className?> :: operator [] ( SizeType index ) const {
    return str[index];
}

inline
const char & <?=$className?> :: at( SizeType index ) const {
    FATALIF( index >= length, "Attempting to access character past the end of a STRING.");

    return str[index];
}


<? ob_start(); ?>

// Hash function
<?  $functions[] = ['Hash', ['@type'], 'base::BIGINT', true, true]; ?>
inline
uint64_t Hash( const @type & str ) {
    return HashString( (void *) str.ToString(), str.Length() );
}

// Checking for Nulls
<?  $functions[] = ['IsNull', ['@type'], 'BASE::BOOL', true, true]; ?>
inline
bool IsNull( const @type & str ) {
    return str.IsNull();
}

#ifdef _HAS_STD_HASH

#include <functional>
// C++11 STL-Compliant hash struct specialization

namespace std {

template <>
class hash<@type> {
    size_t operator () (const @type & key) const {
        return Hash( key );
    }
};

}

#endif // _HAS_STD_HASH
<? $gContent .= ob_get_clean(); ?>

<? return array(
    'kind'             => 'TYPE',
    "system_headers"   => array ( "cstring", "cinttypes", 'iostream', 'cstdlib', 'string' ),
    "user_headers"     => array( "HashFunctions.h", "Constants.h", "Config.h", "Errors.h", "ColumnVarIterator.h" ),
    "complex"          => "ColumnVarIterator< @type >",
    "global_content"   => $gContent,
    'binary_operators' => [ '==', '!=', '>', '<', '>=', '<=' ],
    'constructors'     => $constructors,
    'methods'          => $methods,
    'functions'        => $functions,
    'fixed_size'       => false,
    'describe_json' => DescribeJson('string'),
    'properties'       => [ 'string' ]
);


} // end of function

declareSynonym("base::VARCHAR", "base::STRING");

?>

