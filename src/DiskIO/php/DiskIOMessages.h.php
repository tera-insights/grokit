/**
	This header file contains the messages used by DiskIO.
*/

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
require_once('MessagesFunctions.php');
?>



#ifndef _DISKIO_MESSAGES_H_
#define _DISKIO_MESSAGES_H_

#include "Message.h"
#include "EventProcessor.h"
#include "DistributedCounter.h"
#include "ID.h"
#include "Chunk.h"
#include "QueryExit.h"
#include "QueryTerminationTracker.h"
#include "DiskIOData.h"
#include "Data.h"
#include "ExecEngineData.h"
#include "EEExternMessages.h"

#include <cstdint>

///////////// DISK STATISTICS MESSAGE //////////

/** Message sent by each individual disk to the DiskArray with the
		latest statistics on how fast it works. The statistics consist in
		the estimate for the expected value and variance of the time/page
		each disk takes.

		Arguments:
			diskNo: which disk
			expectation: the expectation of the time
			variance: the variance of the time
*/
<?php
grokit\create_message_type( 'DiskStatistics', [ 'diskNo' => 'int', 'expectation' => 'double', 'variance' => 'double', ], [ ] );
?>



///////////// DISK OPERATION MESSAGE ////////////

/** Message sent by the ChunkReaderWriter to the DiskArray to ask it
		to perform an operation. The notification comes back from the
		HDThreaad that is the last one performing the operation, thus
		saving extra messages between the DiskArray and HDThread.

		Arguments:
			requestID: request identifier (so that the caller knows what is confirmed
			operation: either the DISK_READ or DISK_WRITE macros
			pages: the pages that need to be accessed
*/

<?php
grokit\create_message_type( 'DiskOperation', [ 'requestId' => 'off_t', 'operation' => 'int', ], [ 'requestor' => 'EventProcessor', 'requests' => 'DiskRequestDataContainer', ] );
?>



//////////// MEGA JOB MESSAGE  ///////////

/** Message sent by the DiskArray to each HDThread to set up a chunk read

	Arguments:
		requestID: request identifier (so that the caller knows what is confirmed
		operation: either the DISK_READ or DISK_WRITE macros
		counter: the counter used by all the HDThreads to know when to tell the ChunkReader that the MegaJob is fully processed
		pages: the pages that need to be accessed
*/
<?php
grokit\create_message_type( 'MegaJob',
    [ 'requestId' => 'off_t', 'operation' => 'int', 'counter' => 'DistributedCounter*', ],
    [ 'requestor' => 'EventProcessor', 'requests' => 'DiskRequestDataContainer', ] );
?>


/////////// MEGA JOB FINISHED MESSAGE /////////

/** Message sent by HDThreads when a mega-job is finished

	Arguments:
		requestId: request identifier  copied form the original message
		operation: either the READ or WRITE macros
		counter: the counter used by all the HDThreads to know when to tell the ChunkReader that the MegaJob is fully processed
*/
<?php
grokit\create_message_type( 'MegaJobFinished', [ 'requestId' => 'off_t', 'operation' => 'int', 'counter' => 'DistributedCounter*', ], [ ] );
?>


//////////// CHUNK READ MESSAGE //////////////
/** Message by execution engine to the ChunkReaderWriter to read a chunk.

	Arguments:
	   chunkID: which chunk to read
		 useUncompressed: if set to true, uncompressed data is used, otherwise compressed
		 lineage, request: used for routing the reply inside execEngine
		 dest: destination queryExits
		 colsToProcess: list of (logical,phisical) columns to process
*/

<?php
grokit\create_message_type( 'ChunkRead', [ 'requestor' => 'WayPointID', 'chunkID' => 'off_t', 'useUncompressed' => 'bool', ], [ 'lineage' => 'HistoryList', 'dest' => 'QueryExitContainer', 'token' => 'GenericWorkToken', 'colsToProcess' => 'SlotPairContainer', ] );
?>


//////////// CHUNK WRITE MESSAGE //////////////
/** Same as above but used for writing chunks

	Arguments:
     numTuples: number of tuples in chunk
		 lineage, request: used for routing the reply inside execEngine
		 dest: destination queryExits
		 colsToProcess: list of (logical,phisical) columns to process

*/
<?php
grokit\create_message_type( 'ChunkWrite', [ 'requestor' => 'WayPointID', ], [ 'chunk' => 'Chunk', 'lineage' => 'HistoryList', 'dest' => 'QueryExitContainer', 'token' => 'GenericWorkToken', 'colsToProcess' => 'SlotPairContainer', ] );
?>



//////////// COMMIT METADATA ///////////////////
/* Message sent to the ChunkReaderWriter to tell it to write the current metadata
	 to the Catalog. 		A reply is sent back to the requestor when the operation is done.

	 Arguments:
		requestID: the id of the original request
		append: if true, data is appended, else it overides existing
		requestor: the event processor that gets the ACK

*/
<?php
grokit\create_message_type( 'CommitMetadata', [ 'requestID' => 'off_t', 'append' => 'bool', ], [ 'requestor' => 'EventProcessor', ] );
?>


<?php
grokit\create_message_type( 'Flush', [ ], [ ] );
?>

<?
grokit\create_message_type( 'DeleteContent', [ ], [] );
?>

///////////// CHUNK CLUSTER UPDATE ///////////////
/*	Message sent to the ChunkReaderWrite to tell it to update the clustering
	range for the given chunk.

	Arguments:
		requestor: The WayPoint sending the update
		chunkID: The ID of the chunk to update
		range: The new clustering range
 */
<?
grokit\create_message_type(
	'ChunkClusterUpdate'
	, [
		'requestor' => 'WayPointID',
		'chunkID' => 'ChunkID',
		'range' => 'std::pair<int64_t, int64_t>']
	, [ ]
);
?>

#endif // _DISKIO_MESSAGES_H_
