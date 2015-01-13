<?
// Copyright 2014 Tera Insights, LLC. All Rights Reserved.

function Hash($args, array $t_args = []) {
    $funcName = generate_name('Hash_');
    $retType = lookupType('base::BIGINT');
?>
<?=$retType?> <?=$funcName?>( <?=const_typed_ref_args($args)?> ) {
    uint64_t hashVal = H_b;
<?  foreach($args as $name => $type) { ?>
    hashVal = CongruentHash(<?=$name?>, hashVal);
<?  } // foreach argument ?>
    return static_cast<<?=$retType?>>(hashVal);
}
<?
    return [
        'kind'      => 'FUNCTION',
        'name'      => $funcName,
        'result'    => $retType,
        'system_headers' => [ 'cinttypes' ],
        'user_headers' => [ 'HashFunctions.h' ],
        'deterministic' => true,
    ];
}
?>
