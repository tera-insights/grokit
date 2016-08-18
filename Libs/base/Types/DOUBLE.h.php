<?
//  Copyright 2012 Alin Dobra and Christopher Jermaine
//  Copyright 2013, Tera Insights. All rights reserved

function DOUBLE(){
    // global content
    $gContent = "";
    $functions = [];
    $constructors = [];
?>

typedef double DOUBLE; // use native double

/////////////////
// Aliases

// NO ALIASES

//////////////
// Inline functions

<? ob_start(); ?>
inline void FromString(@type & x, const char* text){
    x = std::strtod(text, nullptr);
}

inline int ToString(const @type & x, char* text){
    // add 1 for the \0
    return 1+sprintf(text,"%1.15g", x);
}


// the hash function
// reinterpret bits as 64 bit int
template<>
inline uint64_t Hash( const @type& val){
    return   *((const uint64_t*)(&val));
}

// Deep copy
inline
void Copy( @type & to, const @type & from ) {
    to = from;
}

<?  $functions[] = [ 'IsNull', ['@type'], 'BASE::BOOL', true, true]; ?>
inline
bool IsNull( @type f ) {
    return isnan(f);
}

<? $gContent .= ob_get_clean(); ?>
//////////////
// Operators

// all operators defined by C++

<?

return array(
    'kind'             => 'TYPE',
    "system_headers"   => array ( "cstdlib", "cstdio", "cinttypes", 'cmath' ),
    "complex"          => false,
    "global_content"   => $gContent,
    'binary_operators' => [ '+', '-', '*', '/', '==', '!=', '>', '<', '>=', '<=' ],
    'unary_operators'  => [ '+', '-' ],
    'constructors'     => $constructors,
    'functions'        => $functions,
    'properties'       => [ 'real', 'numeric', '_primative_' ],
    'describe_json'    => DescribeJson('float'),
    'extras'           => [ 'size.bytes' => 8 ],
    );

} //end of function

// Define the constructors from the various other types.
foreach( [ 'BYTE', 'SMALLINT', 'BIGINT', 'FLOAT', 'INT' ] as $type ) {
    $fullType = 'base::' . $type;
    $call = function($args, $targs = []) use ($fullType) {
        $arg = lookupType($fullType);
        $ret = lookupType('base::DOUBLE');

        return array(
            'kind' => 'FUNCTION',
            'input' => [ $arg ],
            'result' => $ret,
            'deterministic' => true,
        );
    };

    declareFunctionGlobal( 'base', 'DOUBLE', [ $fullType ], $call );
}

declareFunction('DOUBLE', ['GrokitNull'], function($args, $targs = []) {
  $rtype = lookupType('BASE::DOUBLE');
?>
inline
DOUBLE DOUBLE_Null( const GrokitNull & n ) {
    return std::numeric_limits<DOUBLE>::quiet_NaN();
}
<?
  return [
      'kind' => 'FUNCTION',
      'input' => $args,
      'result' => $rtype,
      'deterministic' => true,
      'name' => 'DOUBLE_Null'
      ];
});

?>
