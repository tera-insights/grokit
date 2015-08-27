<?
function Gather(array $t_args, array $inputs, array $outputs) {
    // Class name randomly generated
    $className = generate_name("Gather");

    $outputs = array_combine(array_keys($outputs), $inputs);

    $sys_headers = ['vector'];
    $user_headers = [];
    $lib_headers = [];
    $libraries = ['armadillo'];
    $extra = [];
    $result_type = 'single'
?>

using namespace std;
using tuple = std::tuple<<?=typed($inputs)?>>;

class <?=$className?>;

class <?=$className?> {
 private:
  // The container that gathers the input.
  vector<tuple> items;

  // The tuple to be filled with the current item.
  tuple item;

  // Used for iterating over the container during GetNextResult.
  int return_counter;

 public:
  <?=$className?>(){}

  void AddItem(<?=const_typed_ref_args($inputs)?>) {
<?  foreach (array_keys($inputs) as $index => $name) { ?>
    get<<?=$index?>>(item) = <?=$name?>;
<?  } ?>
    items.push_back(item);
  }

  void AddState(<?=$className?>& other) {
    items.reserve(items.size() + other.items.size());
    items.insert(items.end(), other.items.begin(), other.items.end());
  }

  void Finalize() {
    return_counter = 0;
  }

  void GetResult(<?=typed_ref_args($outputs)?>, int index = 0) const {
<?  foreach (array_keys($outputs) as $index => $name) { ?>
    <?=$name?> = get<<?=$index?>>(items[index]);
<?  } ?>
  }

  int GetCount() const {
    return items.size();
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
        'extra'          => $extra,
        'iterable'       => false,
        'input'          => $inputs,
        'output'         => $outputs,
        'result_type'    => $result_type,
    ];
}
?>
