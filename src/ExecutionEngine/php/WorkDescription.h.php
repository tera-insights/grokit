
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



#ifndef WORK_DESCRIPTION_H
#define WORK_DESCRIPTION_H

#include <string>

#include "Data.h"
#include "GLAData.h"
#include "Chunk.h"
#include "AggStorageMap.h"
#include "HashTable.h"
#include "JoinWayPointID.h"
#include "ExecEngineData.h"
#include "GIStreamInfo.h"
#include "ServiceData.h"
#include "DistributedCounter.h"

// this is the base class for the hierarchy of types that one can send to a CPU
// worker to describe the task that the worker is supposed to complete.  In more
// detail, a CPU worker will always execute a function of type WorkFunc
// (see WorkerMessages.h.m4)... functions of type WorkFunc accept data of type
// WorkDescription.  Sooo, if a specific waypoint wants a function executed, it
// sends a specific WorkFunc to a worker, along with a specific WorkDescription
// object that it has created to parameterize the function

<?php
grokit\create_base_data_type( "WorkDescription", "Data", [ ], [ ] );
?>

<?
grokit\create_data_type("CacheChunkWD", "WorkDescription", [ ], [ 'chunkToProcess' => 'ChunkContainer' ]);
?>

<?
grokit\create_data_type("CompactProcessChunkWD", "WorkDescription", [ 'chunkID' => 'ChunkID', ], [ 'whichQueryExits' => 'QueryExitContainer', 'chunkToProcess' => 'Chunk' ]);
?>

<?php
grokit\create_data_type( "SelectionPreProcessWD", "WorkDescription", [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'requiredStates' => 'QueryToGLASContMap', ] );
?>


<?php
grokit\create_data_type( "SelectionProcessChunkWD", "WorkDescription", [ 'chunkID' => 'ChunkID', ], [ 'whichQueryExits' => 'QueryExitContainer', 'chunkToProcess' => 'Chunk', 'constStates' => 'QueryToGLAStateMap', ] );
?>

<?
grokit\create_data_type(
    'PrintFileObj'
    , 'DataC'
    , [ 'file' => 'FILE *', 'separator' => 'std::string', 'type' => 'std::string' ]
    , [ ]
);
?>

typedef EfficientMap<QueryID, PrintFileObj> QueryToFileMap;
typedef EfficientMap<QueryID, SwapifiedDCptr > QueryToCounters;

// this is the work description for a print
// streams contains file descriptors for all files with query results
<?php
grokit\create_data_type( "PrintWorkDescription", "WorkDescription", [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'streams' => 'QueryToFileMap', 'chunkToPrint' => 'Chunk', 'counters' => 'QueryToCounters', ] );
?>

<?
grokit\create_data_type( 'PrintFinalizeWorkDescription', 'WorkDescription', [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'streams' => 'QueryToFileMap' ] );
?>


// this is a work descrption for a table scan... will likely go away when we move
// past the toy version.  Right now, what this has is a list of all of the query
// exits that should appear in the chunk, as well as what chunk ID we should produce
<?php
grokit\create_data_type( "TableScanWorkDescription", "WorkDescription", [ 'whichChunk' => 'int', 'isLHS' => 'int', ], [ 'whichQueryExits' => 'QueryExitContainer', ] );
?>


// this is the work description for an aggregate operation... there are two types
// of work descriptions.  The first one just aggregates the one chunk that is sent
<?php
grokit\create_data_type( "AggregateOneChunk", "WorkDescription", [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'aggFuncs' => 'AggStorageMap', 'chunkToProcess' => 'Chunk', ] );
?>


// this one finishes up the aggregation process for a specific set of query-exits
<?php
grokit\create_data_type( "FinishAggregate", "WorkDescription", [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'aggFuncs' => 'AggStorageMap', ] );
?>


// this one is for the LHS of a join (the probing side)... note that wayPointID is NOT the
// same thing as the the WayPointID associated with one of the waypoints.  It is a unique int,
// managed by the JoinWayPointImp class, that is associated with each join way point.  It is
// put into the central hash table so that we do not mix tuples from different join waypoints
<?php
grokit\create_data_type( "JoinLHSWorkDescription", "WorkDescription", [ 'wayPointID' => 'int', ], [ 'whichQueryExits' => 'QueryExitContainer', 'chunkToProcess' => 'Chunk', 'centralHashTable' => 'HashTable', ] );
?>


// this one is for the RHS of a join (the hashing side)
<?php
grokit\create_data_type( "JoinRHSWorkDescription", "WorkDescription", [ 'wayPointID' => 'int', ], [ 'whichQueryExits' => 'QueryExitContainer', 'chunkToProcess' => 'Chunk', 'centralHashTable' => 'HashTable', ] );
?>


// this one is for the RHS of a join (the hashing side)
<?php
grokit\create_data_type( "JoinLHSHashWorkDescription", "WorkDescription", [ 'wayPointID' => 'int', ], [ 'whichQueryExits' => 'QueryExitContainer', 'chunkToProcess' => 'Chunk', 'centralHashTable' => 'HashTable', ] );
?>


<?php
grokit\create_data_type( "JoinMergeWorkDescription", "WorkDescription", [ 'wayPointID' => 'WayPointID', 'start' => 'int', 'end' => 'int', ], [ 'whichQueryExits' => 'QueryExitContainer', 'chunksLHS' => 'ContainerOfChunks', 'chunksRHS' => 'ContainerOfChunks', ] );
?>


// work description for the hash table cleaner
<?php
grokit\create_data_type( "HashCleanerWorkDescription", "WorkDescription", [ 'whichSegment' => 'int', ], [ 'centralHashTable' => 'HashTable', 'diskTokenQueue' => 'DiskWorkTokenQueue', 'dyingWayPointsToSend' => 'JoinWayPointIDList', 'dyingWayPointsToHold' => 'JoinWayPointIDList', 'theseQueriesAreDone' => 'QueryExitContainer', 'equivalences' => 'JoinWayPointIDEquivalences', ] );
?>



// Text loader instructions.
// Arguments:
//   loclDictionary: the local dictionary used by this thread
//   stream: the FILE descriptor used by this thread
<?php
grokit\create_data_type( "TextLoaderWorkDescription", "WorkDescription", [ 'stream' => 'FILE*', 'file' => 'std::string', ], [ 'whichQueryExits' => 'QueryExitContainer', ] );
?>


// this is used for the "test" version of the writer... sends the waypoint that this work is being done for
// as well as all of the chunks that have been sent
<?php
grokit\create_data_type( "WriterWorkDescription", "WorkDescription", [ 'whichWayPoint' => 'WayPointID', 'whichSegment' => 'int', ], [ 'extractionList' => 'ExtractionList', ] );
?>



/***** WorkDescriptions *****/

/** work for GLAPreProcessWorkFunc:

    only needs the list of query exits.
*/
<?php
grokit\create_data_type( "GLAPreProcessWD", "WorkDescription", [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'requiredStates' => 'QueryToGLASContMap', ] );
?>


/** work for GLAProcessChunkWorkFunc:

    glaStates: a map and must have an element for each value in whichQueryExits
               if the state exists. Missing entries result in creation of the state.
*/
<?php
grokit\create_data_type( "GLAProcessChunkWD", "WorkDescription", [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'glaStates' => 'QueryToGLAStateMap', 'constStates' => 'QueryToGLAStateMap', 'chunkToProcess' => 'Chunk', 'garbageStates' => 'QueryToGLAStateMap', ] );
?>


/*** work description for GLAMergeStatesWorkFunc
     glaStates contains a list of states for each query
*/
<?php
grokit\create_data_type( "GLAMergeStatesWD", "WorkDescription", [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'glaStates' => 'QueryToGLASContMap', ] );
?>


/*** work description for GLAPreFinalizeWorkFunc
     glaStates contains a map from queryID to GLAState
*/
<?php
grokit\create_data_type( "GLAPreFinalizeWD", "WorkDescription", [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'glaStates' => 'QueryToGLAStateMap', 'constStates' => 'QueryToGLAStateMap', ] );
?>


/*** work description for the finalize function

     fragInfo specifies the fragment number to use for each query that supports fragments

*/
<?php
grokit\create_data_type( "GLAFinalizeWD", "WorkDescription", [ 'fragmentNo' => 'int', ], [ 'whichQueryExit' => 'QueryExit', 'glaState' => 'GLAState', ] );
?>

<?php
grokit\create_data_type(
    "GLAPostFinalizeWD",
    "WorkDescription",
    [ ],
    [
        'whichQueryExits' => 'QueryExitContainer',
        'glaStates'       => 'QueryToGLAStateMap'
    ]
);
?>


/***** Generalized Filter Work Descriptions *****/

<?php
grokit\create_data_type( "GTPreProcessWD", "WorkDescription", [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'requiredStates' => 'QueryToGLASContMap', ] );
?>


<?php
grokit\create_data_type( "GTProcessChunkWD", "WorkDescription", [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'gtStates' => 'QueryToGLAStateMap', 'constStates' => 'QueryToGLAStateMap', 'chunkToProcess' => 'Chunk', ] );
?>


/***** GIST Work Descriptions *****/

// Creates any needed generated constant states and, along with the
// received states, creates the GIST state.
<?php
grokit\create_data_type( "GISTPreProcessWD", "WorkDescription", [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'receivedStates' => 'QueryToGLASContMap', ] );
?>


// Gets a new Global Scheduler from the GIST and uses it to generate the new
// round's local schedulers
<?php
grokit\create_data_type( "GISTNewRoundWD", "WorkDescription", [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'gists' => 'QueryToGLAStateMap', ] );
?>


// Uses the gist state, local scheduler and gla to perform steps on the gist
// until either the local scheduler is exhausted or a timeout is reached.
<?php
grokit\create_data_type( "GISTDoStepsWD", "WorkDescription", [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'workUnits' => 'QueryToGistWorkUnit', ] );
?>


// Merges the GLAs for a query together.
<?php
grokit\create_data_type( "GISTMergeStatesWD", "WorkDescription", [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'glaStates' => 'QueryToGLASContMap', ] );
?>


// Consults with the merged GLA to determine whether or not the GIST should
// go for another round or produce results this round.
<?php
grokit\create_data_type( "GISTShouldIterateWD", "WorkDescription", [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'glaStates' => 'QueryToGLAStateMap', 'gists' => 'QueryToGLAStateMap', ] );
?>


// Produces results for this GIST.
<?php
grokit\create_data_type( "GISTProduceResultsWD", "WorkDescription", [ 'fragmentNo' => 'int', ], [ 'whichOne' => 'QueryExit', 'gist' => 'GLAState', ] );
?>


<?php
grokit\create_data_type( "GIProduceChunkWD", "WorkDescription", [ ], [ 'gi' => 'GLAState', 'stream_info' => 'GIStreamProxy', 'queriesCovered' => 'QueryIDSet' ] );
?>

/***** GSE Work Descriptions *****/

// Pre-processing
<?
grokit\create_data_type( "GSEPreProcessWD", "WorkDescription", [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'requiredStates' => 'QueryToGLASContMap', ] );
?>

// Processing a read-only request
<?
grokit\create_data_type( "GSEProcessWD", "WorkDescription", [ ], [ 'whichQueryExits' => 'QueryExitContainer', 'gseStates' => 'QueryToGLAStateMap', 'constStates' => 'QueryToGLAStateMap', 'request' => 'ServiceData' ] );
?>

/****** Cluster Work Descriptions ******/

// Process Chunk
<?
grokit\create_data_type(
    'ClusterProcessChunkWD',
    'WorkDescription',
    [ ],
    [ 'chunk' => 'Chunk' ]
);
?>

#endif // WORK_DESCRIPTION_H
