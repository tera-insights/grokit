<?
//
//  Copyright 2012 Alin Dobra and Christopher Jermaine, Apache 2.0
//  Copyright 2013, Tera Insights. All rights reserved

function INT(){
    // global content
    $gContent = "";
    $functions = [];
    $constructors = [];

    $nullVal = "-1";
?>

typedef int INT; // use native int

// Inline functions


<?  $constructors[] = [['BASE::NULL'], true, 'INT_Null']; ?>
inline
INT INT_Null( const GrokitNull & n ) {
    return <?=$nullVal?>;
}

<? ob_start(); ?>

inline void FromString(@type & x, const char* text){
    x=atoi(text);
}

inline int ToString(const @type & x, char* text){
    // add 1 for the \0
    return 1+sprintf(text,"%d", x);
}

// The hash function
// we just use conversion to unsigned int
inline uint64_t Hash(const @type x){ return x;}


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

inline
int64_t ClusterValue(const @type & x) {
    return x;
}

<? $gContent .= ob_get_clean(); ?>

//////////////
// Operators

// all operators defined by C++

<?
    return array(
        'kind'             => 'TYPE',
        "system_headers"    => array ( "cstdlib", "cstdio", "cinttypes", 'limits' ),
        "complex"          => false,
        "global_content"   => $gContent,
        'binary_operators' => [ '+', '-', '*', '/', '%', '==', '!=', '>', '<', '>=', '<=' ],
        'unary_operators'  => [ '+', '-' ],
        'constructors'     => $constructors,
        'functions'        => $functions,
        'properties'       => [ 'integral', 'numeric', '_primative_', 'clusterable' ],
        'describe_json'    => DescribeJson('integer'),
        'extras'           => [ 'size.bytes' => 4 ],
    );
}

declareOperator('//', ['base::INT', 'base::INT'], function($args) {
    $l = lookupType('base::INT');
    return [
        'kind'      => 'OPERATOR',
        'name'      => '/',
        'input'     => [ $l, $l ],
        'result'    => $l,
        'deterministic' => true,
    ];
});

// Define the constructors from the various other types.
foreach( [ 'BYTE', 'SMALLINT', 'BIGINT', 'FLOAT', 'DOUBLE' ] as $type ) {
    $fullType = 'base::' . $type;
    $call = function($args, $targs = []) use ($fullType) {
        $arg = lookupType($fullType);
        $ret = lookupType('base::INT');

        return array(
            'kind' => 'FUNCTION',
            'input' => [ $arg ],
            'result' => $ret,
            'deterministic' => true,
        );
    };

    declareFunctionGlobal( 'base', 'INT', [ $fullType ], $call );
}

declareSynonym('BASE::INTEGER', 'BASE::INT');

?>
