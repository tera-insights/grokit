<?
// Copyright 2013 Tera Insights, LLC. All Rights Reserved

function Count( array $t_args, array $input, array $output ) {
    grokit_assert( \count($output) <= 1,
        'Count GLA only produces 1 argument, ' . \count($output) .
        ' outputs were expected.' );

    $asJson = get_default($t_args, 'as.json', false);

    if( $asJson )
        $oType = lookupType('base::JSON');
    else
        $oType = lookupType('base::BIGINT');
    array_set_index( $output, 0, $oType );

    // Don't care what inputs are

    $name = generate_name('Count_');
?>
class <?=$name?> {
    long long unsigned int count;

public:
    <?=$name?>() : count(0) {}
    void AddItem( <?=const_typed_ref_args($input)?> ) { count++; }
    void AddState( <?=$name?> & o ) { count += o.count; }
    void GetResult(<?=$oType?> & _count ) const {
<?  if( $asJson) { ?>
        Json::Value jVal(Json::objectValue);
        jVal["count"] = (Json::Int64) count;
        _count.set(jVal);
<?  } else {  ?>
        _count = count;
<?  } ?>
    }
};
<?
    return [
        'kind'        => 'GLA',
        'name'        => $name,
        'input'       => $input,
        'output'      => $output,
        'result_type' => 'single',
        ];
}
?>
