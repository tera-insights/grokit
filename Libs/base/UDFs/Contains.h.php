<?

function Contains($args, $targs) {
	grokit_assert(\count($args) == 1,
		'Contains supports exactly 1 input, ' . \count($args) . ' given');
	grokit_assert(array_key_exists('values', $targs),
		'Contains() requires a "values" template argument');

	$inputName = 'contains_input';
	$inputType = $args[0];

	$boolType = lookupType('base::bool');

	$typename = generate_name('_ContainsType');
	$funcname = generate_name('Contains');

	$sys_headers = ['cstddef'];

	$use_mct = get_default($targs, 'use.mct', false);

	if( $use_mct ) {
		$sys_headers[] = 'mct/closed-hash-set.hpp';
		$setType = 'mct::closed_hash_set<' . $inputType . ', KeyHash>';
	} else {
		$sys_headers[] = 'unordered_set';
		$setType = 'std::unordered_set<' . $inputType . ', KeyHash>';
	}

	$values = $targs['values'];
	grokit_assert(is_array($values),
		'Contains(): values argument must be an array of strings');

	$quotedValues = [];
	$escapeChars = "\"'\n\r\t\\\0";
	foreach($values as $index => $val) {
		grokit_assert(is_string($val),
			"Contains(): Value at index $index is not a string");

		$quotedValues[] = '"' . addcslashes($val, $escapeChars) . '"';
	}
	$nVals = \count($quotedValues);
?>

class <?=$typename?> {
public:
	struct KeyHash {
		std::size_t operator () (const <?=$inputType?> & val) const {
			return static_cast<std::size_t>(Hash(val));
		}
	};

	using Set = <?=$setType?>;

	// Singleton
	static const <?=$typename?> instance;

private:
	static const char* str_values[<?=$nVals?>];

	Set values;

	<?=$typename?>():
		values()
	{
		<?=$inputType?> temp;
		for( auto str : str_values ) {
			FromString(temp, str);
			values.insert(temp);
		}
	}

public:
	bool exists(const <?=$inputType?> & <?=$inputName?>) const {
		return values.count(<?=$inputName?>) > 0;
	}
};

const <?=$typename?> <?=$typename?>::instance;
const char* <?=$typename?>::str_values[<?=$nVals?>] = {
	<?=implode(", ", $quotedValues)?>	
};

<?=$boolType?> <?=$funcname?>(const <?=$inputType?> & <?=$inputName?>) {
	return <?=$typename?>::instance.exists(<?=$inputName?>);
}

<?

	return [
		'kind' 			=> 'FUNCTION',
		'name'			=> $funcname,
		'input' 		=> $args,
		'result' 		=> $boolType,
		'determinstic'  => true,
		'system_headers' => $sys_headers
	];
}

?>
