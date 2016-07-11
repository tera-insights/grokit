<?
// This GT functions similarly to IntervalMap, except that the input state is a
// GroupBy on top of an Interval_Map GLA. The inputs should include one interval
// entry and a number of keys equal to that used in the GroupBy, in that order.
// The keys are used to look up an inner GLA, which is then used to perform the
// interval join using the first input.

// Template Args:
// keep.missing: Whether to keep input tuples that have no matches. If so, the
//   missing fields are filled in with NULL values.
function Group_By_Interval_Join($t_args, $inputs, $outputs, $states) {
    // Class name is randomly generated.
    $className = generate_name('GroupByIntervalJoin');

    // Processing of template arguments.
    $keepMissing = get_default($t_args, 'keep.missing', false);

    // Processing of input states.
    $states_ = array_combine(['state'], $states);
    $state = $states_['state'];

    // The function used to rename keys.
    $callable = function($key, $prefix) {
        return $prefix . $key;
    };

    // Processing of inputs.
    $keys = array_values($state->get('keys'));
    foreach ($keys as $index => $type)
        $inputKeys["input_key_$index"] = $type;
    grokit_assert(\count($inputs) == \count($keys) + 1,
                  'GroupByIntervalJoin: incorrect number of inputs given.');
    $inputs_ = array_merge(array_combine(['value'], array_slice($inputs, 0, 1)),
                           $inputKeys);
    $innerGLA = $state->get('inner_gla');
    grokit_assert(canConvert($inputs_['value'], $innerGLA ->get('bound')),
                  'IntervalJoin: cannot convert input to bound type.');

    // Processing of outputs.
    $outputs_ = $state->input();
    grokit_assert(\count($outputs) == \count($outputs_),
                  'IntervalJoin: incorrect number of outputs specified.');
    $count = \count($keys);
    foreach (array_values(array_slice($outputs_, 0, $count)) as $i => $type)
        $outputKeys["output_key_$i"] = $type;
    $outputPair = array_combine(['lower', 'upper'],
                                array_slice($outputs_, $count, 2));
    foreach (array_values(array_slice($outputs_, 2 + $count)) as $i => $type)
        $outputPass["output$index"] = lookupType($type);
    $outputs_ = array_merge($outputPair, $outputKeys, $outputPass);
    $outputs = array_combine(array_keys($outputs), $outputs_);

    // A mapping of input name to output name for each key.
    $keyMap = array_combine(array_keys($inputKeys), array_keys($outputKeys));

    $sys_headers  = [];
    $user_headers = [];
    $lib_headers  = [];
    $libraries    = [];
    $properties   = [];
    $extras       = [];
?>

class <?=$className?>;

<?  $constantState = lookupResource(
        'base::Group_By_Interval_Join_Constant_State',
        ['className' => $className, 'state' => $state]
    ); ?>

class <?=$className?> {
 public:
  // The type of the constant state
  using ConstantState = <?=$constantState?>;

  // The type of the object used for matching.
  using ObjectType = ConstantState::InputState::InnerGLA::ObjectType;

 private:
  // The constant state.
  const ConstantState& constant_state;

  // The current and past-the-end iterators.
  ObjectType::const_iterator it, end;

 public:
  <?=$className?>(const ConstantState& state)
      : constant_state(state) {
  }

  bool ProcessTuple(<?=process_tuple_args($inputs_, $outputs_)?>) {
    // The map is searched.
    if (!constant_state.state.Contains(<?=args($inputKeys)?>)) {
      // std::cout << "Missing key: " << input_key_0.addr.asInt << std::endl;
      return false;
    }
    auto& map = constant_state.state.Get(<?=args($inputKeys)?>).GetMap();
    auto it = map.lower_bound({value, value});

    // Copying the keys into the outputs.
    <?=array_template('{val} = {key};', PHP_EOL, $keyMap)?>

    // Copying the bounds to the outputs.
    lower = it->first.first;
    upper = it->first.second;

    // Checking if the map contained a match.
    if (it->first.first <= value && value < it->first.second) {
      // Copying pass-through outputs.
<?  foreach(array_keys($outputPass) as $index => $name) { ?>
      <?=$name?> = std::get<<?=$index?>>(it->second);
<?  } ?>
      return true;
    } else {
<?  if ($keepMissing) { ?>
      // Constructing NULL outputs.
<?      foreach($outputs_ as $name => $type) { ?>
      <?=$name?> = <?=\grokit\getNull($type)?>;
<?      } ?>
      return true;
<?  } else { ?>
      // No match. Output is skipped.
      return false;
<?  } ?>
    }
  }
};

<?
    return [
        'kind'            => 'GT',
        'name'            => $className,
        'generated_state' => $constantState,
        'system_headers'  => $sys_headers,
        'user_headers'    => $user_headers,
        'lib_headers'     => $lib_headers,
        'libraries'       => $libraries,
        'iterable'        => false,
        'input'           => $inputs,
        'output'          => $outputs,
        'result_type'     => 'single',
    ];
}

function Group_By_Interval_Join_Constant_State(array $t_args) {
    // Processing of template arguments.
    $className = $t_args['className'];
    $state     = $t_args['state'];

    $states = ['state' => $state];

    $sys_headers  = [];
    $user_headers = [];
    $lib_headers  = [];
    $libraries    = [];
    $properties   = [];
    $extras       = [];
?>

class <?=$className?>ConstantState {
 public:
  using InputState = <?=$state?>;

 private:
  // The inner object used for matching.
  const InputState& state;

 public:
  friend class <?=$className?>;

  <?=$className?>ConstantState(<?=const_typed_ref_args($states)?>)
      : state(state) {}
};
<?
    return [
        'kind'           => 'RESOURCE',
        'name'           => $className . 'ConstantState',
        'system_headers' => $sys_headers,
        'user_headers'   => $user_headers,
        'lib_headers'    => $lib_headers,
        'libraries'      => $libraries,
        'configurable'   => false,
    ];
}
?>
