<?
//
//  Copyright 2012 Alin Dobra and Christopher Jermaine, Apache 2.0
//  Copyright 2013, Tera Insights. All rights reserved

function FLOAT(){

    $functions = [];
    $constructors = [];
?>

typedef float FLOAT; // use native int

<?  $constructors[] = [['BASE::NULL'], true, 'FLOAT_Null']; ?>
inline
FLOAT FLOAT_Null( const GrokitNull & n ) {
    return std::numeric_limits<FLOAT>::quiet_NaN();
}

<?  ob_start(); ?>

inline void FromString(@type& x, const char* text){
    x=atof(text);
}

inline int ToString(const @type& x, char* text){
    // add 1 for the \0
    return 1+sprintf(text,"%f", x);
}

// the hash function
// interpret as int (same size)
inline uint64_t Hash(const @type val){
    return *( (const unsigned int*)(&val) );
}

// Deep copy
inline
void Copy( @type& to, const @type& from ) {
    to = from;
}

<?  $functions[] = [ 'IsNull', ['@type'], 'BASE::BOOL', true, true]; ?>
inline
bool IsNull( @type f ) {
    return isnan(f);
}

<?  $gContents = ob_get_clean(); ?>

//////////////
// Operators

// all operators defined by C++

<?

return array(
    'kind'             => 'TYPE',
    "system_headers"   => array ( "cstdlib", "cstdio", "cinttypes", 'cmath', 'limits' ),
    "complex"          => false,
    'binary_operators' => [ '+', '-', '*', '/', '==', '!=', '>', '<', '>=', '<=' ],
    'unary_operators'  => [ '+', '-' ],
    'constructors'     => $constructors,
    'functions'        => $functions,
    'properties'       => [ 'real', 'numeric', '_primative_' ],
    'global_content'   => $gContents,
    'describe_json'    => DescribeJson('float'),
    'extras'           => [ 'size.bytes' => 4 ],
    );

} // end of function

// Define the constructors from the various other types.
foreach( [ 'BYTE', 'SMALLINT', 'BIGINT', 'INT', 'FLOAT', 'DOUBLE' ] as $type ) {
    $fullType = 'base::' . $type;
    $call = function($args, $targs = []) use ($fullType) {
        $arg = lookupType($fullType);
        $ret = lookupType('base::FLOAT');

        return array(
            'kind' => 'FUNCTION',
            'input' => [ $arg ],
            'result' => $ret,
            'deterministic' => true,
        );
    };

    declareFunctionGlobal( 'base', 'FLOAT', [ $fullType ], $call );
}

?>

