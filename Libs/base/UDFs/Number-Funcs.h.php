<?

//  Copyright 2012 Alin Dobra and Christopher Jermaine, Apache 2.0
//  Copyright 2013, Tera Insights. All rights reserved

/*
 * This file contains function and operator definitions for the numeric types.
 */

function _makeOpCall($left, $right, $ret, $name = null) {
    $f = function($args, $targs = []) use ($left, $right, $ret, $name) {
        $l = lookupType($left);
        $r = lookupType($right);
        $v = lookupType($ret);
        $info = [
            'kind'      => 'OPERATOR',
            'input'     => [ $l, $r ],
            'result'    => $v,
            'deterministic'     => true,
        ];

        if( !is_null($name) )
            $info['name'] = $name;

        return $info;
    };

    return $f;
}

// Create a function to do this because otherwise all of the variables I use will
// be declared in the global scope because PHP's namespaces are terrible.
function _defineNumericOperators($namespacePrefix, $numericTypes, $ops, $boolOps) {
    // Set up all of the operators between different numeric types.
    for( $leftInd = 0; $leftInd < \count($numericTypes); $leftInd += 1 ) {
        for( $rightInd = 0; $rightInd < $leftInd; $rightInd += 1 ) {
            $left = $namespacePrefix . $numericTypes[$leftInd];
            $right = $namespacePrefix . $numericTypes[$rightInd];
            $ret = $left;

            foreach( $ops as $name => $opval ) {
                if( is_int($name) ) {
                    $v = null;
                    $op = $opval;
                } else {
                    $v = $opval;
                    $op = $name;
                }

                declareOperator( $op, [ $left, $right ], _makeOpCall($left, $right, $ret, $v) );
                declareOperator( $op, [ $right, $left ], _makeOpCall($right, $left, $ret, $v) );
            }
            foreach( $boolOps as $op ) {
                declareOperator( $op, [ $left, $right ], _makeOpCall($left, $right, 'base::bool') );
                declareOperator( $op, [ $right, $left ], _makeOpCall($right, $left, 'base::bool') );
            }
        }
    }
}

_defineNumericOperators(
    'base::',
    [ 'BYTE', 'SMALLINT', 'INT', 'BIGINT', 'FLOAT', 'DOUBLE' ],
    [ '+', '-', '*', '/' ],
    [ '<', '>', '<=', '>=', '==', '!=' ]
);

// Integer-only operators
_defineNumericOperators(
    'base::',
    [ 'BYTE', 'SMALLINT', 'INT', 'BIGINT' ],
    [ '%', '//' => '/' ],
    [ ]
);

declareFunction('ToString', ['BASE::INT'], function() {
    $intType = lookupType('BASE::INT');
    $stringType = lookupType('BASE::STRING');
?>
<?=$stringType?> ToString( <?=$intType?> val ) {
    std::string str = std::to_string(val);
    return <?=$stringType?>(str.c_str());
}
<?
    return [
        'kind'              => 'FUNCTION',
        'input'             => [$intType],
        'result'            => $stringType,
        'deterministic'     => true,
        'system_headers'    => [ 'string' ],
    ];
});

declareFunction('ToString', ['BASE::BIGINT'], function() {
    $intType = lookupType('BASE::BIGINT');
    $stringType = lookupType('BASE::STRING');
?>
<?=$stringType?> ToString( <?=$intType?> val ) {
    std::string str = std::to_string(val);
    return <?=$stringType?>(str.c_str());
}
<?
    return [
        'kind'              => 'FUNCTION',
        'input'             => [$intType],
        'result'            => $stringType,
        'deterministic'     => true,
        'system_headers'    => [ 'string' ],
    ];
});

declareFunction('ToString', ['BASE::SMALLINT'], function() {
    $intType = lookupType('BASE::SMALLINT');
    $stringType = lookupType('BASE::STRING');
?>
<?=$stringType?> ToString( <?=$intType?> val ) {
    std::string str = std::to_string(val);
    return <?=$stringType?>(str.c_str());
}
<?
    return [
        'kind'              => 'FUNCTION',
        'input'             => [$intType],
        'result'            => $stringType,
        'deterministic'     => true,
        'system_headers'    => [ 'string' ],
    ];
});

declareFunction('ToString', ['BASE::BYTE'], function() {
    $intType = lookupType('BASE::BYTE');
    $stringType = lookupType('BASE::STRING');
?>
<?=$stringType?> ToString( <?=$intType?> val ) {
    std::string str = std::to_string(val);
    return <?=$stringType?>(str.c_str());
}
<?
    return [
        'kind'              => 'FUNCTION',
        'input'             => [$intType],
        'result'            => $stringType,
        'deterministic'     => true,
        'system_headers'    => [ 'string' ],
    ];
});
?>
