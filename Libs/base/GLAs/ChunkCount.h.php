<?php
// Copyright 2015 Tera Insights, LLC. All Rights Reserved.

function ChunkCount( array $t_args, array $input, array $output ) {
	$name = generate_name('ChunkCount');

	grokit_assert( \count($output) == 2,
		'ChunkCount only supports 2 outputs');

	$numberName = array_keys($output)[0];
	$countName = array_keys($output)[1];
	$numberType = lookupType('BASE::BIGINT');
	$countType = lookupType('BASE::BIGINT');

	$output[$numberName] = $numberType;
	$output[$countName] = $countType;
?>

class <?=$name?> {
	using map_type = std::unordered_map<<?=$numberType?>, <?=$countType?>>;

	map_type chunkCount;
	<?=$numberType?> curCount;

	map_type::const_iterator multiIter;

public:
	<?=$name?>() : chunkCount(), curCount(0) { }

	void AddItem(<?=const_typed_ref_args($input)?>) {
		curCount++;
	}

	void AddState(const <?=$name?>& other) {
		for (auto elem: other.chunkCount) {
			chunkCount[elem.first] += elem.second;
		}
	}

	void ChunkBoundary() {
		chunkCount[curCount] += 1;
		curCount = 0;
	}

	void Finalize() {
		multiIter = chunkCount.cbegin();
	}

	bool GetNextResult(<?=typed_ref_args($output)?>) {
		if (multiIter != chunkCount.cend()) {
			<?=$numberName?> = multiIter->first;
			<?=$countName?> = multiIter->second;
			multiIter++;
			return true;
		} else {
			return false;
		}
	}
};

<?php
	return [
		'kind'				=> 'GLA',
		'name'				=> $name,
		'system_headers'	=> ['unordered_map'],
		'input'				=> $input,
		'output'			=> $output,
		'result_type'		=> ['multi'],
		'chunk_boundary'	=> true
	];
}

?>