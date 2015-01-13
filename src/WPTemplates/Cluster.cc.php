<?

/* Copyright 2014, Tera Insights. All rights Reserved */

/* function to instantiate a Cluster waypoint */

function ClusterGenerate($wpName, $attr) {
?>

// module specific headers to allow separate compilation
#include "GLAData.h"
#include "Errors.h"

#include <cstdint>
#include <limits>
#include <algorithm>

//+{"kind":"WPF", "name":"Process Chunk", "action":"start"}
extern "C"
int ClusterProcessChunkWorkFunc_<?=$wpName?>(
	WorkDescription &workDescription,
	ExecEngineData &result)
{
	PROFILING2_START;

	// Get the work description
	ClusterProcessChunkWD myWork;
	myWork.swap(workDescription);

	Chunk &input = myWork.get_chunk();

	// Extract column from chunk
	Column clusterCol;
	input.SwapColumn(clusterCol, <?=$attr->slot()?>);
	FATALIF(!clusterCol.IsValid(),
		"Error: Column <?=$attr?> not found in <?=$wpName?>");
	<?=$attr->type()->iterator()?> clusterIter(clusterCol);

	// Extract bitstring iterator from chunk
	BStringIterator queries;
	input.SwapBitmap(queries);

	// Set up the initial interval
	int64_t min = std::numeric_limits<int64_t>::max();
	int64_t max = std::numeric_limits<int64_t>::min();

	uint64_t n_tuples = 0;

	while( !queries.AtEndOfColumn() ) {
		const <?=$attr->type()?>& <?=$attr?> = clusterIter.GetCurrent();

		int64_t value = ClusterValue(<?=$attr?>);
		min = std::min(min, value);
		max = std::max(max, value);

		n_tuples++;

		clusterIter.Advance();
		queries.Advance();
	}

	PROFILING2_END;
	PROFILING2_SINGLE("tpi", n_tuples, "<?=$wpName?>");

	// Package Result
	ClusterProcessChunkRez myRez(min, max);
	myRez.swap(result);

	return WP_PROCESS_CHUNK;
}
//+{"kind":"WPF", "name":"Process Chunk", "action":"end"}

<?
} // end function ClusterGenerate
?>