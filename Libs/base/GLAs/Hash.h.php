<?
function Multi_Hash(array $t_args, array $inputs, array $outputs)
{
    // Class name randomly generated
    $className = generate_name('Hash');

    // Processing of template arguments.
    $split = $t_args['split'];
    $seed = get_default($t_args, 'seed', 0);

    // Processing of inputs.
    $keys = array_slice($inputs, 0, $split);
    $vals = array_slice($inputs, $split);

    // Local names for the inputs are generated.
    foreach (array_values($keys) as $index => $type)
        $inputs_["key$index"] = $type;
    foreach (array_values($vals) as $index => $type)
        $inputs_["val$index"] = $type;

    // Re-assigned using local names for the inputs.
    $keys = array_slice($inputs_, 0, $split);
    $vals = array_slice($inputs_, $split);

    $sys_headers  = ['tuple', 'unordered_map'];
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
  // The seed for the chain hash.
  static const constexpr size_t kSeed = <?=$seed?>;

  // The value type for the map that contains the various values passed through.
  using Tuple = std::tuple<<?=typed($vals)?>>;

  // The keys are placed into one overall key.
  struct Key {
    // The various hashing attributes.
    <?=array_template('{val} {key};', PHP_EOL . '    ', $keys)?>

    // Simple initializer list to copy inner keys.
    Key(<?=const_typed_ref_args($keys)?>)
      : <?=array_template('{key}({key})', ',' . PHP_EOL . '        ', $keys)?> {
    }

    bool operator==(const Key& other) const {
      return <?=array_template('{key} == other.{key}',' && ', $keys)?>;
    }

    bool operator<(const Key& other) const {
<?  foreach ($keys as $name => $type) { ?>
  if (<?=$name?> < other.<?=$name?>)
        return true;
          if (!(<?=$name?> < other.<?=$name?>))
        return false;
<?  } ?>
      return false;
    }

    // The chain-hash across the inner keys.
    size_t ChainHash() const {
      uint64_t hash = kSeed;
      <?=array_template('hash = SpookyHash(Hash({key}), hash);',
                        PHP_EOL . '      ', $keys)?>
      return hash;
    }

    // Getter methods for each inner key.
<?  foreach (array_keys($keys) as $index => $key) { ?>
    const <?=$keys[$key]?>& GetKey<?=$index?>() const {
      return <?=$key?>;
    }
<?  } ?>
  };

  // The structure used to template the map.
  struct HashKey {
    size_t operator()(const Key& key) const {
      return key.ChainHash();
    }
  };

  // The map used, which must be a multimap.
  using Map = mct::closed_hash_map<Key, Tuple, HashKey>;

 private:
  // The container that gathers the input.
  Map map;

 public:
  <?=$className?>() {}

  void AddItem(<?=const_typed_ref_args($inputs_)?>) {
    auto key = Key(<?=args($keys)?>);
    auto val = std::make_tuple(<?=args($vals)?>);
    map.insert(std::make_pair(key, val));
  }

  void AddState(<?=$className?>& other) {
    map.insert(other.map.begin(), other.map.end());
  }

  const Map& GetMap() const {
    return map;
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
