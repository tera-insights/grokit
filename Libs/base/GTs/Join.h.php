<?
function Join_Constant_State(array $t_args)
{
    // Initialization of local variables from template arguments.
    $className = $t_args['className'];
    $states    = $t_args['states'];

    $states_ = array_combine(['mapping'], $states);

    // Return values.
    $sys_headers  = [];
    $user_headers = [];
    $lib_headers  = [];
    $libraries    = [];
?>

class <?=$className?>ConstantState {
 private:
  using Mapping = <?=$states_['mapping']?>;

  // The GroupBy state containing the hash map.
  const Mapping::Map& map;

 public:
  friend class <?=$className?>;

  <?=$className?>ConstantState(<?=const_typed_ref_args($states_)?>)
      : map(mapping.GetMap()) {
  }
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

function Join($t_args, $inputs, $outputs, $states) {
    // Class name randomly generated
    $className = generate_name('Join');

    // Processing of states;
    $states_ = array_combine(['mapping'], $states);

    // Processing of outputs.
    $values = $states_['mapping']->get('vals');
    $outputs = array_combine(array_keys($outputs), $values);

    $sys_headers  = ['map', 'tuple'];
    $user_headers = [];
    $lib_headers  = [];
    $libraries    = [];
    $properties   = ['list'];
    $extra        = [];
    $result_type  = ['multi'];
?>

class <?=$className?>;

<?  $constantState = lookupResource(
        "Join_Constant_State", ['className' => $className, 'states' => $states]
    ); ?>

class <?=$className?> {
 private:
  using ConstantState = <?=$constantState?>;
  using Mapping = ConstantState::Mapping;
  using Iter = Mapping::Map::const_iterator;

  // The GroupBy containing the hash.
  const <?=$constantState?>& constant_state;

  // The iterator used for the multi return.
  Iter it, end;

 public:
  <?=$className?>(const <?=$constantState?>& state)
      : constant_state(state) {
  }

  void ProcessTuple(<?=const_typed_ref_args($inputs)?>) {
    auto key = Mapping::ChainHash(<?=args($inputs)?>);
    auto pair = constant_state.map.equal_range(key);
    it = pair.first;
    end = pair.second;
  }

  bool GetNextResult(<?=typed_ref_args($outputs)?>) {
    if (it == end)
      return false;
<?  foreach (array_keys($outputs) as $index => $name) { ?>
    <?=$name?> = std::get<<?=$index?>>(it->second);
<?  } ?>
    ++it;
    return true;
  }
};

<?
    return [
        'kind'            => 'GT',
        'name'            => $className,
        'system_headers'  => $sys_headers,
        'user_headers'    => $user_headers,
        'lib_headers'     => $lib_headers,
        'libraries'       => $libraries,
        'input'           => $inputs,
        'output'          => $outputs,
        'result_type'     => 'multi',
        'generated_state' => $constantState,
    ];
}
?>
