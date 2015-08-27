<?
//
//  Copyright 2012 Alin Dobra and Christopher Jermaine, Apache 2.0
//  Copyright 2013, Tera Insights. All rights reserved

function BIGINT(){
    // global content
    $gContent = "";
    $functions = [];
    $constructors = [];

    $nullVal = "-1";
?>

typedef int64_t BIGINT; // use native int

// Inline functions

<?  $constructors[] = [['BASE::NULL'], true, 'BIGINT_Null']; ?>
inline
BIGINT BIGINT_Null( const GrokitNull & n ) {
    return <?=$nullVal?>;
}

<? ob_start(); ?>

inline void FromString( @type & x, const char* text){
    x=atol(text);
}

inline int ToString(const @type & x, char* text){
    // add 1 for the \0
    return 1+sprintf(text,"%ld", (long int)x);
}

inline uint64_t Hash(const @type val){
    return val;
}

// Copy function
inline void Copy( @type & to, const @type & from ) {
    to = from;
}

<?  $functions[] = [ 'IsNull', [ '@type' ], 'BASE::BOOL', true, true ]; ?>
inline
bool IsNull( const @type t ) {
    return t == <?=$nullVal?>;
}

<? $gContent .= ob_get_clean(); ?>

//////////////
// Operators

// all operators defined by C++

<? return array(
        'kind'             => 'TYPE',
        "system_headers"    => array ( "cstdlib", "cstdio", "cinttypes", 'limits' ),
        "complex"          => false,
        "global_content"   => $gContent,
        'binary_operators' => [ '+', '-', '*', '/', '%', '==', '!=', '>', '<', '>=', '<=' ],
        'unary_operators'  => [ '+', '-' ],
        'constructors'     => $constructors,
        'functions'        => $functions,
        'properties'       => [ 'integral', 'numeric', '_primative_' ],
        'describe_json'    => DescribeJson('integer'),
        'extras'           => [ 'size.bytes' => 8 ],
    );


} // end of function

declareOperator('//', ['base::BIGINT', 'base::BIGINT'], function($args) {
    $l = lookupType('base::BIGINT');
    return [
        'kind'      => 'OPERATOR',
        'name'      => '/',
        'input'     => [ $l, $l ],
        'result'    => $l,
        'deterministic' => true,
    ];
});

// Define the constructors from the various other types.
foreach( [ 'BYTE', 'SMALLINT', 'INT', 'BIGINT', 'FLOAT', 'DOUBLE' ] as $type ) {
    $fullType = 'base::' . $type;
    $call = function($args, $targs = []) use ($fullType) {
        $arg = lookupType($fullType);
        $ret = lookupType('base::BIGINT');

        return array(
            'kind' => 'FUNCTION',
            'input' => [ $arg ],
            'result' => $ret,
            'deterministic' => true,
        );
    };

    declareFunctionGlobal( 'base', 'BIGINT', [ $fullType ], $call );
}

?>

