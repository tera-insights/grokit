<?
// This GT generates a fixed number of rows per tuple based on a sequence. For
// each value, it generates the sequence (value + initial):(value + distance).
// One sequence per input expression is generated. Additionally, the sequence
// can be incremented by more than 1, as specified by the 'increment' argument.
// Both 'initial' and 'increment' are normalized to have the same sign as the
// 'distance' argument.

// Template Args:
// distance: A non-zero integer.
// increment: A non-negative integer.
// initial: A non-negative integer.

function Range($t_args, $inputs, $outputs) {
    // Class name is randomly generated.
    $className = generate_name('Range');

    // Initialization of local variables from template arguments.
    $distance = $t_args['distance'];
    $increment = get_default($t_args, 'increment', 1);
    $initial = get_default($t_args, 'initial', 0);
    grokit_assert(is_integer($distance) && $distance != 0,
                  'Range: distance should be a non-zero integer.');
    grokit_assert(is_integer($increment) && $increment > 0,
                  'Range: increment should be a positive integer.');
    grokit_assert(is_integer($initial) && $initial > 0,
                  'Range: initial should be a positive integer.');
    // The sign of distance.
    $sign = $distance > 0 ? 1 : -1;

    // There is one extra output in the external outputs to account for the key.
    grokit_assert(\count($outputs) == \count($outputs),
                  'Range: Expected equal number of outputs and inputs.');
    $outputs = array_combine(array_keys($outputs), $inputs);

    $sys_headers  = ['tuple', 'cmath'];
    $user_headers = [];
    $lib_headers  = [];
    $libraries    = [];
    $properties   = [];
    $extras       = [];
?>

class <?=$className?> {
 public:
  // The tuple used to contain the initial values being used.
  using ValueType = std::tuple<<?=typed($inputs)?>>;

  static const constexpr long kDistance = <?=$distance?>;

  static const constexpr long kIncrement = <?=$increment?>;

  static const constexpr long kInitial = <?=$initial?>;

  static const constexpr long kSign = <?=$sign?>;

 private:
  // The initial values for the current item.
  ValueType values;

  // The current distance from the initial values;
  size_t distance;

 public:
  <?=$className?>() {}

  void ProcessTuple(<?=const_typed_ref_args($inputs)?>) {
    values = std::make_tuple(<?=args($inputs)?>);
    distance = kInitial;
  }

  bool GetNextResult(<?=typed_ref_args($outputs)?>) {
    if (std::abs(distance) > std::abs(kDistance))
      return false;
<?  foreach (array_keys($outputs) as $index => $name) { ?>
    <?=$name?> = std::get<<?=$index?>>(values) + kSign * distance;
<?  } ?>
    distance += kIncrement;
    return true;
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
?>
