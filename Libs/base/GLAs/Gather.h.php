<?
function Gather(array $t_args, array $inputs, array $outputs) {
    // Class name randomly generated
    $className = generate_name("Gather");

    // Initialization of local variables from template arguments.
    $initSize = get_default($t_args, 'init.size', 0);

    $outputs = array_combine(array_keys($outputs), $inputs);

    $sys_headers  = ['vector'];
    $user_headers = [];
    $lib_headers  = [];
    $libraries    = [];
    $properties   = ['list'];
    $extra        = [];
    $result_type  = ['multi']
?>

class <?=$className?>;

class <?=$className?> {
 public:
  using Tuple = std::tuple<<?=typed($inputs)?>>;

  using Vector = std::vector<Tuple>;

 private:
  // The container that gathers the input.
  Vector items;

  // The tuple to be filled with the current item.
  Tuple item;

  // Used for iterating over the container during GetNextResult.
  int return_counter;

 public:
  <?=$className?>() {
    items.reserve(<?=$initSize?>);
  }

  void AddItem(<?=const_typed_ref_args($inputs)?>) {
<?  foreach (array_keys($inputs) as $index => $name) { ?>
    std::get<<?=$index?>>(item) = <?=$name?>;
<?  } ?>
    items.push_back(item);
  }

  void AddState(<?=$className?>& other) {
    items.reserve(items.size() + other.items.size());
    items.insert(items.end(), other.items.begin(), other.items.end());
  }

  void GetResult(<?=typed_ref_args($outputs)?>, int index = 0) const {
<?  foreach (array_keys($outputs) as $index => $name) { ?>
    <?=$name?> = std::get<<?=$index?>>(items[index]);
<?  } ?>
  }

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

  int GetCount() const {
    return items.size();
  }

  const Vector& GetList() const {
    return items;
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
