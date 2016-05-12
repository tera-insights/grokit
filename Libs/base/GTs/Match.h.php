<?
// This GT takes in a state that represents a map. It matches the given inputs
// to this state and attaches all of the results. The behaviour of the matching
// is up to the input state. One common application is for an interval join.

// Template Args:
// distance: A non-zero integer.
// increment: A non-negative integer.
// initial: A non-negative integer.
function Match($t_args, $inputs, $outputs, $states) {
    // Class name is randomly generated.
    $className = generate_name('Match');

    $states_ = array_combine(['state'], $states);
    $state = $states['state'];

    $sys_headers  = [];
    $user_headers = [];
    $lib_headers  = [];
    $libraries    = [];
    $properties   = [];
    $extras       = [];
?>

class <?=$className?>;

<?  $constantState = lookupResource(
        'base::Match_Constant_State',
        ['className' => $className, 'state' => $state]
    ); ?>

class <?=$className?> {
 public:
  // The type of the constant state
  using ConstantState = <?=$state?>;

  // The type of the object used for matching.
  using ObjectType = ConstantState::InnerState::ObjectType;

 private:
  // The constant state.
  const ConstantState& constant_state

  // The current and past-the-end iterators.
  ObjectType::const_iterator current, end;

 public:
  <?=$className?>() {}

  void ProcessTuple(<?=const_typed_ref_args($inputs)?>) {
    current = constant_state.state.Begin();
    end = constant_state.state.End();
  }

  bool GetNextResult(<?=typed_ref_args($outputs)?>) {
    if (it == end)
      return false;
<?  foreach(array_keys($outputs) as $index => $name) { ?>
    <?=$name?> = std::get<<?=$index?>>(it->second);
<?  } ?>
  }
};

<?
    return [
        'kind'           => 'GT',
        'name'           => $className,
        'system_headers' => $sys_headers,
        'user_headers'   => $user_headers,
        'lib_headers'    => $lib_headers,
        'libraries'      => $libraries,
        'iterable'       => false,
        'input'          => $inputs,
        'output'         => $outputs,
        'result_type'    => 'multi',
    ];
}

function Match_Constant_State(array $t_args) {
    // Initialization of local variables from template arguments.
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
  using InputState = InputState<?=$state?>;

 private:
  // The inner object used for matching.
  const InputState& state

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
