<?
// Copyright 2014 Tera Insights, LLC. All Rights Reserved.

// Declares a function 'Ord' that takes a string literal and returns the first
// character as a byte.
declareFunction('Ord', [ 'base::STRING_LITERAL' ], function() {
    $inputType = lookupType('base::STRING_LITERAL');
    $outputType = lookupType('base::BYTE');

    $inputs = [ $inputType ];

?>
<?=$outputType?> Ord( const <?=$inputType?> & str ) {
    return str[0];
}
<?
    return [
        'type'              => 'FUNCTION',
        'input'             => $inputs,
        'result'            => $outputType,
        'deterministic'     => true
    ];
});
?>
