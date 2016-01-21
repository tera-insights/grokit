<?
//  Copyright 2014, Tera Insights. All rights reserved

function BYTE() {
    // global content
    $gContent = "";

    $functions = [];
    $constructors = [];

    $nullVal = "-1";
?>
typedef char BYTE;

<?  $constructors[] = [['BASE::NULL'], true, 'BYTE_Null']; ?>
inline
BYTE BYTE_Null( const GrokitNull & n ) {
    return <?=$nullVal?>;
}

<? ob_start(); ?>

inline void FromString(@type & x, const char* text){
    x = (char) atoi(text);
}

inline int ToString(const @type & x, char* text){
    // add 1 for the \0
    return 1+sprintf(text,"%hhd", x);
}

// The hash function
// we just use conversion to unsigned int
template<>
inline uint64_t Hash(const @type& x){ return x;}

// Deep copy
inline
void Copy( @type & to, const @type & from ) {
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

<?
    return array(
        'kind'              => 'TYPE',
        "system_headers"    => array ( "cstdlib", "cstdio", "cinttypes", 'limits' ),
        "complex"           => false,
        "global_content"    => $gContent,
        'binary_operators'  => [ '+', '-', '*', '/', '%', '==', '!=', '>', '<', '>=', '<=' ],
        'unary_operators'   => [ '+', '-' ],
        'constructors'      => $constructors,
        'functions'         => $functions,
        'properties'        => [ 'integral', 'numeric', '_primative_' ],
        'describe_json'     => DescribeJson('integer'),
        'extras'            => [ 'size.bytes' => 1 ]
    );
}

declareOperator('//', ['base::BYTE', 'base::BYTE'], function($args) {
    $l = lookupType('base::BYTE');
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
        $ret = lookupType('base::BYTE');

        return array(
            'kind' => 'FUNCTION',
            'input' => [ $arg ],
            'result' => $ret,
            'deterministic' => true,
        );
    };

    declareFunctionGlobal( 'base', 'BYTE', [ $fullType ], $call );
}

?>
