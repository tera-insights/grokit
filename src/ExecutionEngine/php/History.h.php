
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



#ifndef HISTORY_H
#define HISTORY_H

#include <string>

#include "Data.h"
#include "TwoWayList.h"
#include "ID.h"
#include "TwoWayList.cc"
#include "ChunkID.h"

// this macro defines the history objects that are used to store lineage info
// that follows data objects as they move through the path network.  As a waypoint
// processes a data message, it is free to add a new history object to the end of
// the HistoryList that accompanies the the data through the network.  Then, as
// the data message is either acked or dropped, that history object will be
// retured at a later time so that the waypoint can deal with the ack or drop.
// whichWayPoint tells us which waypoint created the history object
<?php
grokit\create_base_data_type( "History", "DataC", [ 'whichWayPoint' => 'WayPointID', ], [ ] );
?>


// this defines the HistroyList data structure... new History objects are appended
// to this as a data message works its way through the data path graph
typedef TwoWayList <History> HistoryList;

// history specific to an aggregate waypoint
<?php
grokit\create_data_type( "AggHistory", "History", [ ], [ ] );
?>


// history specific to an GLA waypoint
<?php
grokit\create_data_type( "GLAHistory", "History", [ 'whichFragment' => 'int', ], [ ] );
?>


<?php
grokit\create_data_type( "GISTHistory", "History", [ 'whichFragment' => 'int', ], [ ] );
?>


// history specific to the hash table cleaner
<?php
grokit\create_data_type( "HashCleanerHistory", "History", [ 'whichSegment' => 'int', ], [ ] );
?>


// history specific to a table scan waypoint
<?php
grokit\create_data_type( "TableScanHistory", "History", [ 'whichChunk' => 'int', ], [ 'whichExits' => 'QueryExitContainer', ] );
?>


// history specific to a table waypoint
<?php
grokit\create_base_data_type( "TableHistory", "History", [ 'whichChunk' => 'ChunkID', ], [ 'whichExits' => 'QueryExitContainer', ] );
?>


// two types of specific histories for Table
<?php
grokit\create_data_type( "TableReadHistory", "TableHistory", [ ], [ ] );
?>

// this is not used now but might be used in the future
// if writer part of Table neds some extra data
<?php
grokit\create_data_type( "TableWriteHistory", "TableHistory", [ ], [ ] );
?>


// history for text lodaders. Contains info about which stream produced the chunk
<?php
grokit\create_data_type( "TextLoaderHistory", "History", [ 'whichChunk' => 'ChunkID', 'file' => 'std::string', ], [ ] );
?>


<?php
grokit\create_data_type( "GIHistory", "History", [ 'whichChunk' => 'ChunkID', 'file' => 'std::string', ], [ ] );
?>


<?php
grokit\create_data_type( "TileJoinScanHistory", "History", [ 'bucketID' => 'int', 'whichChunk' => 'ChunkID', 'isLHS' => 'bool', 'file' => 'std::string', ], [ 'whichExits' => 'QueryExitContainer', ] );
?>


<?php
grokit\create_data_type( "TileJoinWriteHistory", "History", [ ], [ ] );
?>


<?php
grokit\create_data_type( "TileJoinMergeHistory", "History", [ 'bucketID' => '__uint64_t', ], [ 'chunkIDLHSList' => 'ChunkIDContainer', 'chunkIDRHSList' => 'ChunkIDContainer', 'whichExits' => 'QueryExitContainer', ] );
?>

<?
grokit\create_data_type( "CacheHistory", "History", [ 'whichChunk' => 'uint64_t' ], [ ] );
?>


#endif
