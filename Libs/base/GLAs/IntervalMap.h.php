<?
// This GLA simply accumulates its inputs into an interval map. The first two
// two inputs are used as the bounds for each interval and the rest of them are
// placed into a tuple that matches that interval. The type of the first input
// is taken to be the type used for the bounds, so there must be a conversion
// from the type of the second input that of the first.

// Currently, only non-overlapping intervals are supported without aggregate on
// overlap.

// Template Args:
// use.array: If true, an array is used instead of a tuple. All inputs normally
//   placed into the tuple must have the same type in such a case.
function Interval_Map(array $t_args, array $inputs, array $outputs) {
    // Class name randomly generated
    $className = generate_name('IntervalMap');

    // Processing of inputs.
    $boundType = array_get_index($inputs, 0);
    $otherType = array_get_index($inputs, 1);
    grokit_assert(canConvert($otherType, $boundType),
                  "IntervalMap: cannot convert from $otherType to $boundType");
    foreach (array_slice(array_values($inputs), 2) as $index => $type)
        $innerArgs["val$index"] = $type;
    $inputs_ = array_combine(['lower', 'upper'], array_slice($inputs, 0, 2));
    $inputs_ = array_merge($inputs_, $innerArgs);

    // Processing of template arguments.
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

    $sys_headers  = ['map', 'tuple', 'array', 'utility'];
    $user_headers = [];
    $lib_headers  = [];
    $libraries    = [];
    $properties   = ['map'];
    $extra        = ['bound' => $boundType];
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
  using ValueType = std::tuple<<?=typed($innerArgs)?>>;
<?  } ?>

  // The types used for the bounds of the interval map.
  using BoundType = <?=$boundType?>;
  using Interval = std::pair<BoundType, BoundType>;

  // This comparison operator does not allow for overlapping intervals.
  struct Comparator {
    bool operator()(const Interval& lhv, const Interval& rhv) const {
      return lhv.second < rhv.first;
    }
  };

  // The type used for the interval map.
  using IntervalMap = std::map<Interval, ValueType, Comparator>;

  // The type for the object this GLA builds.
  using ObjectType = IntervalMap;

 private:
  // The container that gathers the input.
  IntervalMap map;

 public:
  <?=$className?>() {}

  void AddItem(<?=const_typed_ref_args($inputs_)?>) {
    ValueType value {<?=args($innerArgs)?>};
    auto interval = std::make_pair(lower, upper);
    map.insert(std::make_pair(interval, value));
  }

  void AddState(<?=$className?>& other) {
    map.insert(other.map.cbegin(), other.map.cend());
  }

  const IntervalMap& GetMap() const {
    return map;
  }

  const ObjectType& GetObject() const {
    return map;
  }

  IntervalMap::const_iterator Find(BoundType key) const {
    return map.find(std::make_pair(key, key));
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
