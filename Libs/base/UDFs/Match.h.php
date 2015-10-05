<?

function Match($inputs, $args) {
    // Processing of template arguments.
    $pattern = get_first_key($args, ['pattern', 0], 'Match: no pattern given.');
    grokit_assert(is_string($pattern), 'Match: pattern should be a string.');

    // Processing of inputs.
    $count = \count($inputs);
    grokit_assert($count == 1, "Match supports exactly 1 input. $count given");
    $inputs_ = array_combine(['input'], $inputs);
    $input = $inputs_['input']->name();
    grokit_assert(in_array($input, ['BASE::STRING_LITERAL', 'BASE::STRING']),
                  "Match: input should be a string (literal). $input given.");

    $result = lookupType('BASE::BOOL');

    $className = generate_name('Pattern');
    $functName = generate_name('Matcher');

    $userHeaders = ['PatternMatcherOnig.h'];
?>

class <?=$className?> {
 public:
  static PatternMatcherOnig pattern;
};

PatternMatcherOnig <?=$className?>::pattern("<?=$pattern?>");

<?=$result?> <?=$functName?>(<?=const_typed_ref_args($inputs_)?>) {
<?  if ($input == 'BASE::STRING_LITERAL') { ?>
  return  <?=$className?>::pattern.Match(input);
<?  } else { ?>
  return  <?=$className?>::pattern.Match(input.ToString());
<?  } ?>
}

<?
    return [
        'kind'          => 'FUNCTION',
        'name'          => $functName,
        'input'         => $inputs,
        'result'        => $result,
        'deterministic' => true,
        'user_headers'  => $userHeaders,
    ];
}
?>
