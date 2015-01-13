<?
// Copyright 2014 Tera Insights, LLC. All Rights Reserved.

/**
 *  A GLA that counts the number of distinct elements by keeping track of the
 *  distinct elements.
 *
 *  Unless an exact count of the distinct is absolutely needed, consider using
 *  an approximation of the distinct, such as a Bloom Filter.
 */
function CountDistinct( array $t_args, array $input, array $output ) {
    grokit_assert(\count($output) == 1,
        'CountDistinct should have only 1 output, ' . \count($output) . 'given');

    $outputName = array_keys($output)[0];
    $outputType = array_get_index($output, 0);
    if( is_null($outputType) ) $outputType = lookupType('BASE::BIGINT');
    $output[$outputName] = $outputType;

    grokit_assert($outputType->is('numeric'), 'CountDistinct output must be numeric!');

    $useMCT = get_default($t_args, 'use.mct', true);
    $keepHashes = get_default($t_args, 'mct.keep.hashes', false);
    $initSize = get_default($t_args, 'init.size', 65536);
    $nullCheck = get_default($t_args, 'null.check', false);

    grokit_assert(is_bool($useMCT), 'CountDistinct use.mct argument must be boolean');
    grokit_assert(is_integer($initSize), 'Distinct init.size argument must be an integer');
    grokit_assert($initSize > 0, 'Distinct init.size argument must be positive');
    grokit_assert(is_bool($keepHashes), 'CountDistinct mct.keep.hashes argument must be boolean');

    $distTmpArgs =  [
        'use.mct' => $useMCT,
        'init.size' => $initSize,
        'mct.keep.hashes' => $keepHashes,
        'null.check' => $nullCheck,
    ];
    $gla = lookupGLA('BASE::DISTINCT', $distTmpArgs, $input, $input);

    $className = generate_name('CountDistinct');
?>
class <?=$className?> {
    using Distinct = <?=$gla->value()?>;

    Distinct distinctGLA;

public:

    <?=$className?>(void):
        distinctGLA()
    { }

    ~<?=$className?>(void) { }

    void AddItem(<?=const_typed_ref_args($input)?>) {
        distinctGLA.AddItem(<?=args($input)?>);
    }

    void AddState(<?=$className?> & o) {
        distinctGLA.AddState(o.distinctGLA);
    }

    void GetResult(<?=$outputType?> & <?=$outputName?>) {
        <?=$outputName?> = distinctGLA.get_countDistinct();
    }
};
<?
    return [
        'kind'      => 'GLA',
        'name'      => $className,
        'input'     => $input,
        'output'    => $output,
        'result_type' => 'single',
    ];
}
?>
