<?
// This GT is used to simulate a GroupBy on data that its chunk sorted by key.
// When a key is only present in a single chunk and all appearances of the key
// are consecutive, the grouping for that key can be processed and outputted
// immediately. This strategy removes the need for a large state to hold every
// state at once because each process must only ever keep a single group in its
// state. The Segmenter GLA can also simulate this, but doing so requires both
// merging states and several passes over this data. This approach has no merges
// and makes only two passes over each chunk.

// The weakness of this algorithm is the strict input requirements. These can be
// relaxed by placing a GroupBy GLA after this GT with the same key. Doing so
// allows the key to appear in multiple chunks and non-consecutive tuples.
// However, this approach will not work for every GLA and will decrease the
// benefits of this GT. It should be considered on a case-by-case basis only.

// The input to this GT requires a single key. To simulate a GroupBy on multiple
// keys, simply use a GroupBy on the other keys as the inner GLA.

// The inner GLA result type must be single or multiple result, not fragment.
// The latter is not allowed because the workers for it and the GT would fight
// over the system resources.

// Iterable GLAs could be supported with current framework, but are not.

// Template Args:
// GLA: The inner GLA to use.

function Streaming_GroupBy($t_args, $inputs, $outputs) {
    // Class name is randomly generated.
    $className = generate_name('AdjustableBernoulli');

    // Separating the inputs into the key and inner GLA arguments.
    foreach (array_keys(array_slice($inputs, 1)) as $index => $key)
        $args["arg_$index"] = $inputs[$key];
    $inputs_ = array_combine(array_merge(['key'], array_keys($args)), $inputs);
    $keyType = $inputs_['key'];

    // Processing the inner GLA.
    $innerGLA = $t_args['aggregate'];
    grokit_assert(is_gla($innerGLA),
                  'Non-GLA specified as aggregate for Streaming GroupBy');
    // The inner GLA is instantiated.
    $innerGLA = $innerGLA->apply(array_slice($inputs, 1),
                                 array_slice($outputs, 1));
    grokit_error_if($innerGLA->iterable(),
                    'Iterable inner GLA given for Streaming GroupBy');
    $resultType = get_first_value($innerGLA->result_type(), ['multi', 'single']);
    $innerState = $innerGLA->state();
    $hasState = !is_null($innerState);
    $declareState = $hasState ? "InnerState inner_state;" : "";
    $constructState = $hasState ? "inner_state" : "";

    // Processing the outputs.
    $outputs_ = ['key' => $keyType];
    foreach (array_values($innerGLA->output()) as $index => $type)
        $outputs_["output_$index"] = $type;
    $innerOutputs = array_slice($outputs_, 1);
    $outputs = array_combine(array_keys($outputs), $outputs_);

    // There is one extra output in the external outputs to account for the key.
    grokit_assert(\count($innerOutputs) + 1 == \count($outputs),
                  'Incorrect number of outputs given for Streaming GroupBy');

    $sys_headers  = ['vector'];
    $user_headers = [];
    $lib_headers  = [];
    $libraries    = [];
    $properties   = [];
    $extras       = [];
?>

class <?=$className?> {
 public:
  // The inner GLA being used.
  using InnerGLA = <?=$innerGLA?>;

  // The type for the key values.
  using KeyType = <?=$keyType?>;

<?  if ($hasState) { ?>
  // The type for the inner state.
  using InnerState = <?=$innerState?>;
<?  } ?>

 private:
  // The value of the key for the current group.
  KeyType current_key;

  // The list of chunk-based indices for the last tuple in each group.
  std::vector<uint64_t> group_indices;

  // The state for the inner GLA.
  InnerGLA* inner_gla;

  // The current iteration;
  uint32_t iteration;

  // The 1-based index for a tuple relative to the current chunk.
  uint64_t tuple_index;

  // The 0-based index for the current group.
  uint64_t group_index;

<?  if ($resultType == 'single') { ?>
  // A boolean encodign whether the output has been returned.
  bool should_output;
<?  } ?>

 public:
  // The constructor is empty. The various fields are set-up per chunk.
  <?=$className?>() {}

  // Each data structure is reset.
  void StartChunk() {
    group_indices.clear();
    <?=$declareState?>
    inner_gla = new InnerGLA(<?=$constructState?>);
    iteration = 0;
    tuple_index = 0;
<?  if ($resultType == 'single') { ?>
    should_output = true;
<?  } ?>

  }

  // The first iteration only computes the group boundaries. Later iterations
  // perform the aggregation.
  void ProcessTuple(<?=const_typed_ref_args($inputs_)?>) {
    // The group is advanced if the previous tuple was the last in its group.
    if (iteration != 0 && tuple_index == group_indices[group_index]) {
      group_index++;
      <?=$declareState?>
      inner_gla = new InnerGLA(<?=$constructState?>);
    }
    tuple_index++;
    if (iteration == 0) {
      if (tuple_index == 1) {
        // The key is initialized.
        current_key = key;
        // std::cout << "First key obtained." << std::endl;
      } else if (key != current_key) {
        // std::cout << "Group added." << std::endl;
        // The previous tuple was the last in its group.
        group_indices.push_back(tuple_index - 1);
        current_key = key;
      }
    } else {
      // The arguments for the inner GLA are passed through.
      inner_gla->AddItem(<?=args($args)?>);

      if (tuple_index == group_indices[group_index]) {
        // The key is saved for the coming output.
        current_key = key;
<?  if ($resultType == 'multi') { ?>
        // The result for the inner GLA is readied for output.
        inner_gla->Finalize();
<?  } ?>
      }
    }
  }

  bool GetNextResult(<?=typed_ref_args($outputs_)?>) {
    if (iteration > 0 && tuple_index == group_indices[group_index]) {
      key = current_key;
<?  if ($resultType == 'multi') { ?>
      return inner_gla->GetNextResult(<?=args($innerOutputs)?>);
<?  } else { // $resultType is 'single'?>
      if (should_output)
        inner_gla->GetResult(<?=args($innerOutputs)?>);
      return !(should_output = !should_output);
<?  } ?>
    } else {
      return false;
    }
  }

  bool ShouldIterate() {
    // The last group is recorded.
    group_indices.push_back(tuple_index);

    group_index = 0;
    tuple_index = 0;
    return 1 == ++iteration;
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
        'iterable'       => true,
        'input'          => $inputs,
        'output'         => $outputs,
        'result_type'    => 'multi',
    ];
}
?>
