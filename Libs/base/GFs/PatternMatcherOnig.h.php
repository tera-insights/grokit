<?

function PatternMatcherOnig($t_args, $inputs) {

    grokit_assert(\count($inputs) == 1, 'PatternMatcherOnig GF only supports 1 input!');

    $pattern = get_first_key($t_args, [ 'pattern' ]);

    $inName = array_keys($inputs)[0];
    $inType = array_get_index($inputs, 0);

    $inTypeString = $inType->name();

    $validTypes = [ 'BASE::STRING_LITERAL' ];
    grokit_assert( in_array($inTypeString, $validTypes), 'Unsupported input type ' . $inTypeString );

    $className = generate_name('PatternMatcherOnigGF');
?>

class <?=$className?> {

    PatternMatcherOnig matcher;

public:
    <?=$className?>() :
        matcher("<?=$pattern?>")
    { }

    bool Filter( const <?=$inType?> & <?=$inName?> ) {
        return matcher.Match(<?=$inName?>);
    }
};

<?
    return [
        'kind'          => 'GF',
        'name'          => $className,
        'input'         => $inputs,
        'user_headers'  => [ 'PatternMatcherOnig.h' ]
    ];
}

?>
