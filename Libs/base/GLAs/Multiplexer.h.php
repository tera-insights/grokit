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
function Multiplexer(array $t_args, array $inputs, array $outputs) {
    $className = generate_name('Multiplexer');
    if (\count($inputs) == 0) {
        grokit_assert(array_key_exists('input', $t_args), 'No inputs specified for Multiplexer');
        $inputs = $t_args['input'];

        foreach( $t_args['inputs'] as $name => &$type) {
            if (is_identifier($type)) {
                $type = lookupType(strval($type));
            }

            grokit_assert(is_datatype($type), 'Only types may be specified as inputs to Multiplexer.');
        }

        $inputs = ensure_valid_names($inputs, 'multi_input');
    }

    $glas = get_first_key($t_args, [ 'glas', 0 ]);
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
    $iterable = [];

    foreach ($glas as $name => $glaInfo) {
        grokit_assert(is_array($glaInfo), 'Template argument \'glas\' must be an array');

        grokit_assert(array_key_exists('gla', $glaInfo), "No GLA given for glas[$name]");
        grokit_assert(array_key_exists('inputs', $glaInfo), "No inputs given for glas[$name]");
        grokit_assert(array_key_exists('outputs', $glaInfo), "No outputs given for glas[$name]");

        $gla = $glaInfo['gla'];
        $glaInAtts = $glaInfo['inputs'];
        $glaOutAtts = $glaInfo['outputs'];

        grokit_assert(is_gla($gla), "Non-GLA given for glas[$name][gla]");
        grokit_assert(is_array($glaInAtts), "Non-array given for inputs for gla $name");
        grokit_assert(is_array($glaOutAtts), "Non-array given for outputs for gla $name");

        $glaInAtts = array_map('strval', $glaInAtts);
        $glaOutAtts = array_map('strval', $glaOutAtts);

        $glaName = "innerGLA_" . $name;

        $glaInputs[$glaName] = [];
        $glaOutputs[$glaName] = [];

        foreach ($glaInAtts as $att) {
            grokit_assert(array_key_exists($att, $inputs), "Input $att for GLA $name not found in inputs");
            $glaInputs[$glaName][$att] = $inputs[$att];
        }

        foreach ($glaOutAtts as $att) {
            grokit_assert(array_key_exists($att, $outputs), "Output $att for GLA $name not found in outputs");
            grokit_assert(!in_array($att, $usedOutputs), "Output $att used by multiple GLAs");
            $usedOutputs[] = $att;
            $glaOutputs[$glaName][$att] = $outputs[$att];
        }

        $gla = $gla->apply($glaInputs[$glaName], $glaOutputs[$glaName]);
        $myGLAs[$glaName] = $gla;
        $glaRez[$glaName] = get_first_value($gla->result_type(), ['multi', 'single', 'state']);

        $libraries = array_merge($libraries, $gla->libraries());

        if ($glaRez[$glaName] == 'state') {
            grokit_assert(\count($glaOutputs[$glaName]) == 1,
                          "GLA $glaName is produced as state, and thus must have exactly 1 output.");
            $stateType = lookupType('base::STATE', ['type' => $gla]);
            $glaOutputs[$glaName] = array_combine(array_keys($glaOutputs[$glaName]), [$stateType]);
        } else {
            grokit_assert(\count($glaOutputs[$glaName]) == \count($gla->output()),
                          "GLA $glaName produces different number of outputs than expected");
            $glaOutputs[$glaName] = array_combine(array_keys($glaOutputs[$glaName]), $gla->output());
        }

        // Set types for our output
        foreach ($glaOutputs[$glaName] as $attName => $type)
            $outputs[$attName] = $type;

        $glaReqStates[$glaName] = $gla->req_states();
        $glaGenStates[$glaName] = $gla->state();

        if ($gla->iterable()) {
            grokit_assert(!$gla->intermediates(),
                          'Multiplexer does not support iterable GLAs with intermediate results');
            $iterable[] = $glaName;
        }
    }

    // Removing null results from state information.
    $glaReqStates = array_filter($glaReqStates);
    $glaGenStates = array_diff($glaGenStates, [null]);

    // The code for the initializer list is constructed.
    $initList = array_template(',' . PHP_EOL . '{key}(state.{key}_state)', '', $glaGenStates);

    $isIterable = \count($iterable) > 0;
    $libraries = array_unique($libraries);
    $extra = ['glas' => $myGLAs];
?>

class <?=$className?>_Iterator {
  bool _gotResultsOnce;
  bool _valid;

<?  foreach ($myGLAs as $name => $type) { ?>
  <?=$type?>* it_<?=$name?>;
<?  } // foreach inner gla ?>

 public:
  <?=$className?>_Iterator()
      : _gotResultsOnce(false),
        _valid(false),
        <?=array_template('it_{key}(nullptr)', ',' . PHP_EOL, $myGLAs)?> {}

  <?=$className?>_Iterator(<?=typed_ref_args($myGLAs)?>)
      : _gotResultsOnce(false),
        _valid(true),
        <?=array_template('it_{key}(&{key})', ',' . PHP_EOL, $myGLAs)?> {
<?  foreach ($myGLAs as $name => $type) { ?>
<?      if ($glaRez[$name] == 'multi') { ?>
    <?=$name?>.Finalize();
<?      } ?>
<?  } ?>
  }

  <?=$className?>_Iterator(const <?=$className?>_Iterator& other) = default;

  bool GetNextResult( <?=typed_ref_args($outputs)?>) {
    FATALIF(!_valid, "Tried to get results from an invalid iterator.");

    bool ret = !_gotResultsOnce;
    _gotResultsOnce = true;

<?  foreach ($myGLAs as $name => $type) { ?>
<?      if ($glaRez[$name] == 'multi') { ?>
    ret |= it_<?=$name?>->GetNextResult(<?=args($glaOutputs[$name])?>);
<?      } ?>
<?  } ?>

    if (ret) {
<?  foreach ($myGLAs as $name => $type) { ?>
<?      if ($glaRez[$name] == 'single') { ?>
      it_<?=$name?>->GetResult(<?=args($glaOutputs[$name])?>);
<?      } else if ($glaRez[$name] == 'state') { ?>
<?          $stateVar = array_keys($glaOutputs[$name])[0]; ?>
<?          $stateType = $glaOutputs[$name][$stateVar]; ?>
      <?=$stateVar?> = <?=$stateType?>(it_<?=$name?>);
<?      } // if inner GLA is state ?>
<?  } // foreach inner gla ?>
    }

    return ret;
  }
};

class <?=$className?>;

<?  $constantState = lookupResource(
        'base::Multiplexer_Constant_State',
        ['gen.states' => $glaGenStates, 'class.name' => $className,
         'req.states' => $glaReqStates, 'iterable' => $iterable]
    ); ?>

class <?=$className?> {
 public:
  using Iterator = <?=$className?>_Iterator;
  using ConstantState = <?=$constantState?>;
  <?=array_template('using GLA{key} = {val};', PHP_EOL, array_values($myGLAs))?>

 private:
  Iterator multiIterator;

  // The inner GLAs
  <?=array_template('{val} {key};', PHP_EOL, $myGLAs)?>

  // The multiplexer constant state containing the inner GLAs' constant states.
  const ConstantState& const_state_;

 public:
  <?=$className?>(const ConstantState& state)
    : const_state_(state)<?=$initList?> {}

  void AddItem(<?=const_typed_ref_args($inputs)?>) {
    // Call AddItem individually on each GLA.
<?  foreach ($myGLAs as $gName => $gType) { ?>
<?      if ($gType->iterable()) { ?>
    if (!const_state_.<?=$gName?>_done)
<?      } ?>
    <?=$gName?>.AddItem(<?=args($glaInputs[$gName])?>);
<?  } // foreach inner gla ?>
  }

  void AddState(<?=$className?> & other) {
    // Call AddState individually on each GLA.
<?  foreach ($myGLAs as $gName => $gType)  { ?>
<?      if ($gType->iterable()) { ?>
    if (!const_state_.<?=$gName?>_done)
<?      } ?>
    <?=$gName?>.AddState(other.<?=$gName?>);
<?  } // foreach inner gla ?>
  }

<?  if ($isIterable) { ?>
  bool ShouldIterate(ConstantState& modible_state) {
    bool finished = true;
    // Each iterable inner GLA is checked. If it already finished, we continue.
    // Otherwise, whether it just finished is checked instead and the result is
    // updated into the corresponding boolean field in the constant state.
<?      foreach ($myGLAs as $gName => $gType) { ?>
<?          if ($gType->iterable()) { ?>
    if (!modible_state.<?=$gName?>_done)
      finished &= modible_state.<?=$gName?>_done
                = !<?=$gName?>.ShouldIterate(modible_state.<?=$gName?>_state);
<?          } ?>
<?      } ?>
    return !finished;
  }
<?  } ?>

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

<?  foreach (array_keys($myGLAs) as $index => $name) { ?>
  const <?=$myGLAs[$name]?>& GetGLA<?=$index?>() const {
    return <?=$name?>;
  }
<?  } ?>
};

<?
    return [
        'kind'            => 'GLA',
        'name'            => $className,
        'input'           => $inputs,
        'output'          => $outputs,
        'result_type'     => $resultType,
        'iterable'        => $isIterable,
        'intermediates'   => false,
        'generated_state' => $constantState,
        'libraries'       => $libraries,
        'configurable'    => $configurable,
        'extra'           => $extra,
    ];
} // end function Multiplexer

// Template Arguments:
// class.name: The name of the associated multiplexer GLA.
// req.states: A name to array mapping, containing the required states for each
//   inner GLA. Inner GLAs without required states should not be present.
// gen.states: A name to type mapping, containing the generated state for each
//   inner GLA. Inner GLAs without a generated state should not be present.
// iterable: An array containing the name of each iterable GLA.
function Multiplexer_Constant_State($t_args) {
    $className = $t_args['class.name'];
    $reqStates = $t_args['req.states'];
    $genStates = $t_args['gen.states'];
    $iterable = $t_args['iterable'];

    // The required states are collapsed into a single array.
    $states = [];
    foreach ($reqStates as $states)
        $states = array_merge($constructorArgs, $states);

    // Check that required states are only given for GLAs with constant states.
    $missing = array_diff_key($reqStates, $genStates);
    grokit_assert(\count($missing) == 0,
                  'Required states given for GLAs without constant states.');

    // This is a map of names of inner GLAs to their constructor arguments code.
    $args = array_map('const_typed_ref_args', $genStates);

    // The code for the initializer list is constructed.
    $init = \count($genStates)
          ?   PHP_EOL . ': '
            . array_template('{key}_state({val})', ',' . PHP_EOL, $args)
            . array_template(',' . PHP_EOL . '{val}_done(false)', '', $iterable)
          : '';

?>

class <?=$className?>ConstantState {
 private:
  // The constant states for for the inner GLAs.
  <?=array_template('{val} {key}_state;', PHP_EOL, $genStates)?>

  // The booleans recording whether each iterable GLA is finished.
  <?=array_template('bool {val}_done;', PHP_EOL, $iterable)?>

 public:
  friend class <?=$className?>;

  <?=$className?>ConstantState(<?=const_typed_ref_args($states)?>)<?=$init?> {}
};

<?
    return [
        'kind'           => 'RESOURCE',
        'name'           => $className . 'ConstantState',
        'configurable'   => false,
    ];
}
?>
