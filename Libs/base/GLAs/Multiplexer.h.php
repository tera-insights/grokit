<?
// Copyright 2013 Tera Insights, LLC. All Rights Reserved.
//
// Author: Christopher Dudley

/*
 *  Template Arguments:
 *
 *      [R] 'glas':         A list of GLAs to multiplex. Must contain 3 keys:
 *                          'gla' = the GLA itself
 *                          'inputs' = the inputs to the GLA
 *                          'outputs' = the outputs to the GLA
 */
function Multiplexer( array $t_args, array $inputs, array $outputs ) {
    $className = generate_name('Multiplexer');
    if( \count($inputs) == 0 ) {
        grokit_assert(array_key_exists('input', $t_args), 'No inputs specified for Multiplexer');
        $inputs = $t_args['input'];

        foreach( $t_args['inputs'] as $name => &$type ) {
            if( is_identifier($type) ) {
                $type = lookupType(strval($type));
            }

            grokit_assert(is_datatype($type), 'Only types may be specified as inputs to Multiplexer.');
        }

        $inputs = ensure_valid_names($inputs, 'multi_input');
    }

    $glas = get_first_key($t_args, [ 'glas', 0 ] );
    grokit_assert(\count($glas) > 0, 'No GLAs specified for Multiplexer.');

    $myGLAs = [];
    $glaInputs = [];
    $glaOutputs = [];
    $resultType = 'multi';

    $usedOutputs = [];
    $libraries = [];

    $glaGenStates = [];
    $glaReqStates = [];

    $configurable = false;
    $constArgs = [];
    $genStates = [];
    $reqStates = [];
    $iterable = null;

    foreach( $glas as $name => $glaInfo ) {
        grokit_assert(is_array($glaInfo), 'Template argument \'glas\' must be an array');

        grokit_assert(array_key_exists('gla', $glaInfo), 'No GLA given for glas[' . $name . ']');
        grokit_assert(array_key_exists('inputs', $glaInfo), 'No inputs given for glas[' . $name . ']');
        grokit_assert(array_key_exists('outputs', $glaInfo), 'No outputs given for glas[' . $name . ']');

        $gla = $glaInfo['gla'];
        $glaInAtts = $glaInfo['inputs'];
        $glaOutAtts = $glaInfo['outputs'];

        grokit_assert(is_gla($gla), 'Non-GLA given for glas[' . $name . '][gla]');
        grokit_assert(is_array($glaInAtts), 'Non-array given for inputs for gla ' . $name );
        grokit_assert(is_array($glaOutAtts), 'Non-array given for outputs for gla ' . $name );

        $glaInAtts = array_map('strval', $glaInAtts);
        $glaOutAtts = array_map('strval', $glaOutAtts);

        $glaName = "innerGLA_" . $name;

        $glaInputs[$glaName] = [];
        $glaOutputs[$glaName] = [];

        foreach( $glaInAtts as $att ) {
            grokit_assert(array_key_exists($att, $inputs), 'Input ' . $att . ' for GLA ' . $name . ' not found in inputs');
            $glaInputs[$glaName][$att] = $inputs[$att];
        }

        foreach( $glaOutAtts as $att ) {
            grokit_assert(array_key_exists($att, $outputs), 'Output ' . $att . ' for GLA ' . $name . ' not found in outputs');
            grokit_assert(!in_array($att, $usedOutputs), 'Output ' . $att . ' used by multiple GLAs');
            $usedOutputs[] = $att;
            $glaOutputs[$glaName][$att] = $outputs[$att];
        }

        //fwrite(STDERR, "Inputs for GLA " . $glaName . ": " . print_r($glaInputs[$glaName], true) . PHP_EOL );
        //fwrite(STDERR, "Outputs for GLA " . $glaName . ": " . print_r($glaOutputs[$glaName], true) . PHP_EOL );
        $gla = $gla->apply($glaInputs[$glaName], $glaOutputs[$glaName]);
        $myGLAs[$glaName] = $gla;
        $glaRez[$glaName] = get_first_value($gla->result_type(), [ 'multi', 'single', 'state' ]);

        $libraries = array_merge($libraries, $gla->libraries());

        if( $glaRez[$glaName] == 'state' ) {
            grokit_assert( \count($glaOutputs[$glaName]) == 1, "GLA $glaName is produced as state, and thus must have exactly 1 output.");
            $stateType = lookupType('base::STATE', [ 'type' => $gla] );
            $glaOutputs[$glaName] = array_combine(array_keys($glaOutputs[$glaName]), [ $stateType ] );
        } else {
            grokit_assert(\count($glaOutputs[$glaName]) == \count($gla->output()), 'GLA ' . $glaName . ' produces different number of outputs than expected');
            $glaOutputs[$glaName] = array_combine(array_keys($glaOutputs[$glaName]), $gla->output());
        }

        // Set types for our output
        foreach( $glaOutputs[$glaName] as $attName => $type ) {
            $outputs[$attName] = $type;
        }

        if( is_null($iterable) )
            $iterable = $gla->iterable();
        else
            grokit_assert($iterable == $gla->iterable(), 'Multiplexer does not support mixing iterable and non-iterable GLAs');

        $glaReqStates[$glaName] = $gla->req_states();
        foreach($gla->req_states() as $rstate) {
            $reqStates[] = $rstate;
        }

        $glaGenStates[$glaName] = $gla->state();
        // TODO: Support constant states
        grokit_assert(!$gla->has_state(), 'Multiplexer currently does not support constant states.');
    }

    $libraries = array_unique($libraries);
    $extra = ['glas' => $myGLAs];
?>

class <?=$className?> {
<?  foreach( $myGLAs as $name => $type ) { ?>
    <?=$type?> <?=$name?>;
<?  } // foreach inner gla ?>

    class Iterator {

        bool _gotResultsOnce;
        bool _valid;

<?  foreach( $myGLAs as $name => $type ) { ?>
        <?=$type?> * it_<?=$name?>;
<?  } // foreach inner gla ?>

    public:
        Iterator(void) : _gotResultsOnce(false), _valid(false),
            <?=array_template('it_{key}(nullptr)', ', ', $myGLAs)?>

        { }

        Iterator(<?=typed_ref_args($myGLAs)?>) : _gotResultsOnce(false), _valid(true),
            <?=array_template('it_{key}(&{key})', ', ', $myGLAs)?>

        {
<?
    foreach( $myGLAs as $name => $type ) {
        if( $glaRez[$name] == 'multi' ) {
?>
            <?=$name?>.Finalize();
<?
        } // if inner GLA is multi
    } // foreach inner gla
?>
        }

        Iterator( const Iterator & other) = default;

        ~Iterator() { }

        bool GetNextResult( <?=typed_ref_args($outputs)?> ) {
            FATALIF(!_valid, "Tried to get results from an invalid iterator.");

            bool ret = !_gotResultsOnce;
            _gotResultsOnce = true;

<?
    foreach( $myGLAs as $name => $type ) {
        if( $glaRez[$name] == 'multi' ) {
?>
            ret |= it_<?=$name?>->GetNextResult(<?=args($glaOutputs[$name])?>);
<?
        } // if inner GLA is multi
    } // foreach inner gla ?>

            if( ret ) {
<?  foreach( $myGLAs as $name => $type ) { ?>
<?      if( $glaRez[$name] == 'single' ) { ?>
                it_<?=$name?>->GetResult(<?=args($glaOutputs[$name])?>);
<?      } // if inner GLA is single
        else if( $glaRez[$name] == 'state'  ) {
            $stateVar = array_keys($glaOutputs[$name])[0];
            $stateType = $glaOutputs[$name][$stateVar];
?>
                <?=$stateVar?> = <?=$stateType?>(it_<?=$name?>);
<?      } // if inner GLA is state ?>
<?  } // foreach inner gla ?>
            }

            return ret;
        }
    };

    Iterator multiIterator;

public:
    <?=$className?>() { }
    ~<?=$className?>() { }

    void AddItem(<?=const_typed_ref_args($inputs)?>) {
        // Call AddItem individually on each GLA.
<?  foreach( $myGLAs as $gName => $gType ) { ?>
        <?=$gName?>.AddItem(<?=args($glaInputs[$gName])?>);
<?  } // foreach inner gla ?>
    }

    void AddState( <?=$className?> & other ) {
        // Call AddState individually on each GLA.
<?  foreach( $myGLAs as $gName => $gType )  { ?>
        <?=$gName?>.AddState(other.<?=$gName?>);
<?  } // foreach inner gla ?>
    }

    void Finalize() {
        multiIterator = Iterator(<?=args($myGLAs)?>);
    }

    bool GetNextResult(<?=typed_ref_args($outputs)?>) {
        return multiIterator.GetNextResult(<?=args($outputs)?>);
    }

    void GetResult(<?=typed_ref_args($outputs)?>) {
        Finalize();
        GetNextResult(<?=args($outputs)?>);
    }

<?  foreach( array_keys($myGLAs) as $index => $name) { ?>
    const <?=$myGLAs[$name]?>& GetGLA<?=$index?>() {
      return <?=$name?>;
    }
<?  } ?>
};

<?
    return array(
        'kind'         => 'GLA',
        'name'         => $className,
        'input'        => $inputs,
        'output'       => $outputs,
        'result_type'  => $resultType,
        'libraries'    => $libraries,
        'configurable' => $configurable,
        'extra'        => $extra,
    );
} // end function Multiplexer
?>
