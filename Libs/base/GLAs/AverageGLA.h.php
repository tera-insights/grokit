<?
// Copyright 2013 Tera Insights, LLC. All Rights Reserved

function Average( array $t_args, array $input, array $output ) {
    $className = generate_name('Average');
    grokit_assert( \count($input) == \count($output),
        'Average must have the same number of inputs and outputs');

    $outToIn = [];
    $internalTypes = [];
    $internalInit = [];

    reset($output);
    foreach( $input as $name => $type ) {
        $outKey = key($output);
        $outToIn[$outKey] = $name;
        if( $type->is('numeric') ) {
            $internalTypes[$name] = 'long double';
            $internalInit[$name] = '0.0';
        } else {
            $internalTypes[$name] = $type;
            $internalInit[$name] = '';
        }

        if( is_null(current($output)) ) {
            if( $type->is('numeric') ) {
                $output[$outKey] = lookupType('base::DOUBLE');
            } else {
                $output[$outKey] = $type;
            }
        }

        next($output);
    }

    $countType = 'uint64_t';

    $debug = get_default( $t_args, 'debug', 0 );
?>
class <?=$className?> {
private:

    <?=$countType?> count; // keeps the number of tuples aggregated

<?  foreach( $internalTypes as $name => $type ) { ?>
    <?=$type?> sum_<?=$name?>;
<?  } // foreach internal value ?>

public:
    <?=$className?>() :
        count(0)
<? foreach($internalInit as $name => $init) { ?>
        , sum_<?=$name?>(<?=$init?>)
<?  } // foreach internal initializer ?>
    {}

    void AddItem(<?=const_typed_ref_args($input)?>) {
        count++;

<?  foreach($input as $name => $type) { ?>
        sum_<?=$name?> += <?=$name?>;
<?  } // foreach input ?>
    }

    void AddState(<?=$className?>& o){
        count += o.count;
<?  foreach($input as $name => $type) { ?>
        sum_<?=$name?> += o.sum_<?=$name?>;
<?  } // foreach input ?>
    }

    // we only support one tuple as output
    void GetResult(<?=typed_ref_args($output)?>){
        if( count > 0 ) {
<?  foreach($output as $name => $type) {
        $inName = $outToIn[$name];
?>
            <?=$name?> = (sum_<?=$inName?>) / count;
<?  } // foreach output ?>
        } else {
<?  foreach($output as $name => $type) { ?>
            <?=$name?> = sum_<?=$inName?>;
<?  } // foreach output ?>
        }
    }

};

<?
    $sys_headers = [ 'cinttypes' ];
    if( $debug > 0 ) {
        $sys_headers[] = 'iostream';
        $sys_headers[] = 'sstream';
    }

    return  array (
            'kind'           => 'GLA',
            'name'           => $className,
            'system_headers' => $sys_headers,
            'input'          => $input,
            'output'         => $output,
            'result_type'    => 'single',
        );

}
?>
