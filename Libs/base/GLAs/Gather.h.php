<?
// This GLA simply accumulates its inputs into a single vector to be used as a
// state. Each row of the input is placed into a tuple which is then appended to
// the end of the vector. Vectors are combined across different processes simply
// by concatenation with no particular order.

// Template Arguments:
// init.size: The initial capacity for the vector.
// use.array: If true, an array is used instead of a tuple. All inputs must have
//   the same type in such a case.

// Resources:
// vector: vector
// tuple: tuple, make_tuple
// array: array
// algorithm: min
function Gather(array $t_args, array $inputs, array $outputs) {
    // Class name randomly generated
    $className = generate_name('Gather');

    // Processing of template arguments.
    $initSize = get_default($t_args, 'init.size', 0);
    $useArray = get_default($t_args, 'use.array', false);
    $fragSize = get_default($t_args, 'fragment.size', 2000000);

    if ($useArray) {
        $innerType = array_get_index($inputs, 0);
        foreach ($inputs as $type)
             grokit_assert($innerType == $type,
                           'Gather: array must contain equivalent types.');
        $numInputs = \count($inputs);
    }

    $outputs = array_combine(array_keys($outputs), $inputs);

    $sys_headers  = ['vector', 'tuple', 'array', 'algorithm'];
    $user_headers = [];
    $lib_headers  = [];
    $libraries    = [];
    $properties   = ['list'];
    $extra        = [];
    $result_type  = ['fragment', 'multi']
?>

class <?=$className?>;

class <?=$className?> {
 public:
  // The number of objects per fragment.
  static constexpr std::size_t kFragmentSize = <?=$fragSize?>;

<?  if ($useArray) { ?>
  // The length of the array, i.e. the number of inputs.
  static const constexpr size_t kLength = <?=$numInputs?>;

  // The type for the array, which all inputs are convertible to.
  using InnerType = <?=$innerType?>;

  // The type of items gathered.
  using ValueType = std::array<InnerType, kLength>;
<?  } else { ?>
  // The type of items gathered.
  using ValueType = std::tuple<<?=typed($inputs)?>>;
<?  } ?>

  // The container for the items being gathered.
  using Vector = std::vector<ValueType>;

  // The iterator used for the fragments, a pair of size types. The integers
  // represent the current and last index for the items in the fragment.
  using Iterator = std::pair<std::size_t, std::size_t>;

 private:
  // The container that gathers the input.
  Vector items;

  // The tuple to be filled with the current item.
  ValueType item;

  // Used for iterating over the container during GetNextResult.
  int return_counter;

 public:
  <?=$className?>() {
    items.reserve(<?=$initSize?>);
  }

  void AddItem(<?=const_typed_ref_args($inputs)?>) {
<?  if ($useArray) { ?>
    items.push_back({<?=args($inputs)?>});
<?  } else { ?>
    items.push_back(std::make_tuple(<?=args($inputs)?>));
<?  } ?>
  }

  void AddState(<?=$className?>& other) {
    items.reserve(items.size() + other.items.size());
    items.insert(items.end(), other.items.begin(), other.items.end());
  }

  // The GLA does not use a single result type. This means that this is never
  // implicitly called. It is only provided for manual usage.
  void GetResult(<?=typed_ref_args($outputs)?>, int index = 0) const {
<?  foreach (array_keys($outputs) as $index => $name) { ?>
    <?=$name?> = std::get<<?=$index?>>(items[index]);
<?  } ?>
  }

  // The methods for the multi result type.
  void Finalize() {
    return_counter = 0;
  }

  bool GetNextResult(<?=typed_ref_args($outputs)?>) {
    if (return_counter == items.size())
      return false;
    GetResult(<?=args($outputs)?>, return_counter);
    return_counter++;
    return true;
  }

  // The methods for the fragment result type.
  // This returns the number of items dividied by the fragment size, rounded up.
  int GetNumFragments() {
    return (items.size() / kFragmentSize) + (items.size() % kFragmentSize != 0);
  }

  Iterator* Finalize(int fragment) const {
    std::size_t lower = kFragmentSize * fragment;
    std::size_t upper = std::min(lower + kFragmentSize, items.size());
    return new Iterator(upper, lower);
  }

  bool GetNextResult(Iterator* it, <?=typed_ref_args($outputs)?>) const {
    if (it->first == it->second)
      return false;
    GetResult(<?=args($outputs)?>, it->first++);
    return true;
  }

  // Methods for accessing the GLA when used as a state.
  std::size_t GetCount() const {
    return items.size();
  }

  const Vector& GetList() const {
    return items;
  }
};

using <?=$className?>_Iterator = <?=$className?>::Iterator;

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
