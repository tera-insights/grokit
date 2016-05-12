<?
// This GLA simply accumulates its inputs into an interval map. The first two
// two inputs are used as the bounds for each interval and the rest of them are
// placed into a tuple that matches that interval. The type of the first input
// is taken to be the type used for the bounds, so there must be a conversion
// from the type of the second input that of the first.

// Template Args:
// use.array: If true, an array is used instead of a tuple. All inputs normally
//   placed into the tuple must have the same type in such a case.
function IntervalMap(array $t_args, array $inputs, array $outputs) {
    // Class name randomly generated
    $className = generate_name('IntervalMap');

    // Processing of inputs.
    $boundType = array_get_index($inputs, 0);
    $otherType = array_get_index($inputs, 1);
    grokit_assert(canConvert($otherType, $boundType),
                  "IntervalMap: cannot convert from $otherType to $boundType");
    foreach (array_slice($inputs, 2) as $index => $type)
        $innerArgs["val$index"] = $type;
    $inputs_ = array_combine(['lower', 'upper'], array_slice($inputs, 0, 2));
    $inputs_ = array_merge($inputs_, $innerArgs);

    // Initialization of local variables from template arguments.
    $initSize = get_default($t_args, 'init.size', 0);
    $useArray = get_default($t_args, 'use.array', false);

    if ($useArray) {
        $innerType = array_get_index($inputs, 2);
        foreach (array_slice($inputs, 3) as $type)
             grokit_assert($innerType == $type,
                           'IntervalMap: array must contain equivalent types.');
        $numInputs = \count($inputs);
    }

    // The code used to build the array or tuple.
    $args = args($innerArgs);
    $item = $useArray ? '{' . $args . '}' : 'std::make_tuple(' . $args . ')';

    $sys_headers  = ['boost/icl/interval_map.hpp', 'tuple', 'array'];
    $user_headers = [];
    $lib_headers  = [];
    $libraries    = [];
    $properties   = ['map'];
    $extra        = [];
    $result_type  = ['state']
?>

class <?=$className?>;

class <?=$className?> {
 public:
<?  if ($useArray) { ?>
  static const constexpr size_t kLength = <?=$numInputs?>;

  using InnerType = <?=$innerType?>;

  using ValueType = std::array<InnerType, kLength>;
<?  } else { ?>
  using ValueType = std::tuple<<?=typed($inputs)?>>;
<?  } ?>

  // The type used for the bounds of the interval map.
  using BoundType = <?=$bound?>

  // The type used for the interval map.
  using IntervalMap = boost::icl::interval_map<BoundType, ValueType>;

  // The type for the object this GLA builds.
  using ObjectType = IntervalMap;

 private:
  // The container that gathers the input.
  IntervalMap map;

 public:
  <?=$className?>() {}

  void AddItem(<?=const_typed_ref_args($inputs_)?>) {
    ValueType value {<?=args($innerArgs)?>};
    auto interval = boost::icl::interval<BoundType>::closed_interval(lower, upper);
    map.add(std::make_pair(interval, value));
  }

  void AddState(<?=$className?>& other) {
    for (auto it = other.map.begin(); it != other.map.end(); ++it)
      map.add(std::make_pair(it->first, it->second));
  }

  const IntervalMap& GetMap() const {
    return map;
  }

  const ObjectType& GetObject() const {
    return map;
  }

  ObjectType::const_iterator Begin(Bound value) {
    return map.lower_bound(value);
  }

  ObjectType::const_iterator End(Bound value) {
    return map.upper_bound(value);
  }
};

<?
    return [
        'kind'           => 'GLA',
        'name'           => $className,
        'system_headers' => $sys_headers,
        'user_headers'   => $user_headers,
        'lib_headers'    => $lib_headers,
        'libraries'      => $libraries,
        'properties'     => $properties,
        'extra'          => $extra,
        'iterable'       => false,
        'input'          => $inputs,
        'output'         => $outputs,
        'result_type'    => $result_type,
    ];
}
?>
