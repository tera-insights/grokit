<?
function Join_Constant_State(array $t_args)
{
    // Initialization of local variables from template arguments.
    $className = $t_args['className'];
    $states    = $t_args['states'];

    // Values to be used in C++ code.
    $state = array_keys($states)[0];
    $class = $states[$state];

    // Return values.
    $sys_headers = [];
    $user_headers = [];
    $lib_headers = [];
    $libraries = [];
?>

class <?=$className?>ConstantState {
 private:
  using Map = <?=$class?>;

  // The GroupBy state containing the hash map.
  const Map& map;

 public:
  friend class <?=$className?>;

  <?=$className?>ConstantState(<?=const_typed_ref_args($states)?>)
      : map(<?=$state?>) {
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

function Join(array $t_args, array $inputs, array $outputs, array $states) {
    // Setting output type
    $state = array_get_index($states, 0);
    $outputs = array_combine(array_keys($outputs), $state->get('inner_gla')->output());

    // Class name is randomly generated.
    $className = generate_name("JoinGT");

    // Return values.
    $sys_headers = [];
    $user_headers = [];
    $lib_headers = [];
    $libraries = [];
?>

class <?=$className?>;

<?  $constantState = lookupResource(
        "Join_Constant_State", ['className' => $className, 'states' => $states]
    ); ?>

class <?=$className?> {
 private:
  using ConstantState = <?=$constantState?>;
  using Map = ConstantState::Map;
  using Key = Map::Key;

  // The GroupBy containing the hash.
  const <?=$constantState?>& constant_state;

  // The key for the current tuple being processed.
  Key key;

  // Whether this tuple has already been given output. Each tuple can only
  // be given at most a single output.
  int remaining;

 public:
  <?=$className?>(const <?=$constantState?>& state)
      : constant_state(state) {
  }

  void ProcessTuple(<?=const_typed_ref_args($inputs)?>) {
    key = Key(<?=args($inputs)?>);
    if (constant_state.map.Contains(key))
      remaining = constant_state.map.Get(key).GetCount();
    else
      remaining = 0;
  }

  bool GetNextResult(<?=typed_ref_args($outputs)?>) {
    if (remaining == 0)
      return false;
    remaining--;
    constant_state.map.Get(key).GetResult(<?=args($outputs)?>, remaining);
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
