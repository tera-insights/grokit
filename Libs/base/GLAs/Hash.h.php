<?
function Multi_Hash(array $t_args, array $inputs, array $outputs)
{
    // Class name randomly generated
    $className = generate_name('Hash');

    // Processing of template arguments.
    $split = $t_args['split'];

    // Processing of inputs.
    $keys = array_slice($inputs, 0, $split);
    $vals = array_slice($inputs, $split);

    $sys_headers  = ['map', 'tuple', 'unordered_map'];
    $user_headers = [];
    $lib_headers  = ['HashFct.h'];
    $libraries    = [];
    $properties   = [];
    $extra        = ['keys' => $keys, 'vals' => $vals];
    $result_type  = ['state'];
?>

class <?=$className?>;

class <?=$className?> {
 public:
  // The standard hashing for Grokit is used, which returns a uint64_t.
  using Key = uint64_t;

  // The value type for the map that contains the various values passed through.
  using Tuple = std::tuple<<?=typed($vals)?>>;

  // The map used, which must be a multimap.
  using Map = std::unordered_multimap<uint64_t, Tuple>;

 private:
  // The container that gathers the input.
  Map map;

 public:
  <?=$className?>() {}

  void AddItem(<?=const_typed_ref_args($inputs)?>) {
    auto key = ChainHash(<?=args($keys)?>);
    auto val = std::make_tuple(<?=args($vals)?>);
    map.insert(std::make_pair(key, val));
  }

  void AddState(<?=$className?>& other) {
    map.insert(other.map.begin(), other.map.end());
  }

  const Map& GetMap() const {
    return map;
  }

  static uint64_t ChainHash(<?=const_typed_ref_args($keys)?>) {
    uint64_t offset = 1;
<?  foreach (array_keys($keys) as $name) { ?>
    offset = CongruentHash(Hash(<?=$name?>), offset);
<?  } ?>
    return offset;
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
