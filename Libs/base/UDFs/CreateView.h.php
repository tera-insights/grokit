<?

function CreateView($args, $targs) {
	grokit_assert(\count($args) == 1,
		'CreateView supports exactly 1 input');

	$type = $args[0];

	grokit_assert($type->is('array'),
		'CreateView cannot create view on non-array type');

	$innerType = $type->get('type');
	$size = $type->get('size');

	$viewType = lookupType(
		'BASE::FixedArrayView',
		['type' => $innerType, 'size' => $size]);

	$funcname = generate_name('CreateView_');

?>

<?=$viewType?> <?=$funcname?>( const <?=$type?> &array ) {
	return <?=$viewType?>(array.data());
}

<?
	return [
		'kind' 			=> 'FUNCTION',
		'name' 			=> $funcname,
		'input' 		=> $args,
		'result' 		=> $viewType,
		'deterministic' => false
	];
}
?>