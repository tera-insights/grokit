
<?php
//
//  Copyright 2013 Tera Insights LLC
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
?>
<?php
require_once('DataFunctions.php');
?>



#ifndef EE_DATA_H
#define EE_DATA_H

#include <cstdio>

#include "Data.h"
#include "Chunk.h"
#include "JoinWayPointID.h"
#include "HashData.h"
#include "GLAData.h"
#include "HashTableSegment.h"
#include "Tokens.h"
#include "History.h"
#include "GIStreamInfo.h"
#include "ServiceData.h"

#include <cstdint>

// this file has all of the data types that can be sent downstream thru the
// data path graph

// this macro defines the "ExecEngineData" class, which is a generic container
// for data that is sent downstream from one waypoint to another to be processed.
<?php
grokit\create_base_data_type( "ExecEngineData", "DataC", [ ], [ ] );
?>


// this is the most common type of EEData object: one that contains a single chunk
<?php
grokit\create_data_type( "ChunkContainer", "ExecEngineData", [ ], [ 'myChunk' => 'Chunk', ] );
?>


// A datatype used for caching rejected chunks, while keeping track of its
// history.
<?php
grokit\create_data_type( "CachedChunk", "DataC", [ ], [ 'myChunk' => 'ChunkContainer', 'lineage' => 'HistoryList', 'whichExits' => 'QueryExitContainer', ] );
?>


// another type of EEData used to transport states between waypoints
<?php
grokit\create_data_type( "StateContainer", "ExecEngineData", [ ], [ 'source' => 'WayPointID', 'whichQuery' => 'QueryExit', 'myState' => 'GLAState', ] );
?>


// this is what is returned by a join worker that has put data into the hash table.
// It lists a small sample of the collisions that were found to happen
<?php
grokit\create_data_type( "JoinHashResult", "ExecEngineData", [ ], [ 'sampledQueries' => 'HashSegmentSample', ] );
?>


// this is what comes out of the hash table extraction waypoint
<?php
grokit\create_data_type( "ExtractionContainer", "ExecEngineData", [ 'whichSegment' => 'int', ], [ 'diskTokenQueue' => 'DiskWorkTokenQueue', 'result' => 'ExtractionList', ] );
?>


<?php
grokit\create_data_type( "ExtractionResult", "ExecEngineData", [ 'whichSegment' => 'int', ], [ 'newSegment' => 'HashTableSegment', 'result' => 'ExtractionContainer', ] );
?>


// return data type for Bulk Loader. Need to pass more stuff back
// Arguments:
//   loclDictionary: the local dictionary used by this thread
//   stream: the FILE descriptor used by this thread
//   noTuples: the number of tuples produced by this thread
<?php
grokit\create_data_type( "TextLoaderResult", "ExecEngineData", [ 'stream' => 'FILE*', 'noTuples' => 'off_t', ], [ 'myChunk' => 'Chunk', ] );
?>




/****** Return types from GLA Process chunk *******/

/** Results from preprocessing */
<?php
grokit\create_data_type(
	"GLAPreProcessRez",
	"ExecEngineData",
	[ ],
	[
		'constStates' => 'QueryToGLAStateMap',
		'produceIntermediates' => 'QueryIDSet'
	]
);
?>

// Result from chunk processing
<?
grokit\create_data_type( "GLAProcessRez", "ExecEngineData", [ ], [ 'glaStates' => 'QueryToGLAStateMap', 'chunk' => 'Chunk' ] );
?>

/** Results containing  GLAStates */
<?php
grokit\create_data_type( "GLAStatesRez", "ExecEngineData", [ ], [ 'glaStates' => 'QueryToGLAStateMap', ] );
?>


/** special version used by GLA merge */
<?php
grokit\create_data_type(
	"GLAStatesFrRez"
	, "ExecEngineData"
	, [ ]
	, [
		'glaStates' => 'QueryToGLAStateMap',
		'constStates' => 'QueryToGLAStateMap',
		'fragInfo' => 'QueryIDToInt',
		'queriesToIterate' => 'QueryIDSet', ]
);
?>


/***** Return types for Generalized Transforms *****/
<?php
grokit\create_data_type( "GTPreProcessRez", "ExecEngineData", [ ], [ 'constStates' => 'QueryToGLAStateMap', ] );
?>


<?php
grokit\create_data_type( "GTProcessChunkRez", "ExecEngineData", [ ], [ 'filters' => 'QueryToGLAStateMap', 'chunk' => 'Chunk', ] );
?>


/***** Return types for Selection and Generalized Filters *****/
<?php
grokit\create_data_type(
	"SelectionPreProcessRez"
	, "ExecEngineData"
	, [ ]
	, [ 'constStates' => 'QueryToGLAStateMap' ]
);
?>


/***** Return types for GISTs *****/

// Preprocessing will generate constant states not received from other
// waypoints and initialize the GIST state, and return them.
<?php
grokit\create_data_type(
	"GISTPreProcessRez"
	, "ExecEngineData"
	, [ ]
	, [
		'genStates' => 'QueryToGLAStateMap',
		'gists' => 'QueryToGLAStateMap',
		'produceIntermediates' => 'QueryIDSet'
	  ]
);
?>


// To start the new round, a global scheduler will be obtained from the GIST
// state, which will be used to generate the local schedulers for this round.
// The GLAs for each local scheduler will also be created, and bundled together
// in work units.
<?php
grokit\create_data_type( "GISTNewRoundRez", "ExecEngineData", [ ], [ 'workUnits' => 'QueryToGistWUContainer', ] );
?>


// Steps will be performed on the work units in this step, and they may or may
// not completely finish their work due to timeouts.
// For any finished work, the local scheduler is deallocated and just the
// GLA is returned
<?php
grokit\create_data_type( "GISTDoStepRez", "ExecEngineData", [ ], [ 'unfinishedWork' => 'QueryToGistWorkUnit', 'finishedWork' => 'QueryToGLAStateMap', ] );
?>


// MergeGLA work function simply uses the GLAStatesRez result type.

// Returns whether or not the GIST should go for another round. If not, it will
// have information regarding the number of fragments to be produced by the
// GIST.
<?php
grokit\create_data_type( "GISTShouldIterateRez", "ExecEngineData", [ ], [ 'fragInfo' => 'QueryIDToInt', 'queriesToIterate' => 'QueryIDSet', ] );
?>


// ProduceResults work function will produce a ChunkContainer

// Result of a GI reading data from a file.
<?php
grokit\create_data_type( "GIProduceChunkRez", "ExecEngineData", [ ], [ 'stream' => 'GIStreamProxy', 'gi' => 'GLAState', 'chunk' => 'ChunkContainer', ] );
?>


/***** Return types for GSEs *****/

// Results from preprocessing
<?
grokit\create_data_type( "GSEPreProcessRez", "ExecEngineData", [ ], [ 'constStates' => 'QueryToGLAStateMap', ] );
?>

// Results from processing
<?
grokit\create_data_type( "GSEProcessRez", "ExecEngineData", [ ], [ 'gseStates' => 'QueryToGLAStateMap', 'result' => 'ServiceData' ] );
?>

/***** Return types for Cluster WP ******/

<?
grokit\create_data_type(
	"ClusterProcessChunkRez",
	"ExecEngineData",
	[ 'min' => 'int64_t', 'max' => 'int64_t' ],
	[ ]
);
?>

#endif
