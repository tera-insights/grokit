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



#ifndef GLA_DATA_H
#define GLA_DATA_H

#include <cinttypes>

#include "Data.h"
#include "Chunk.h"
#include "TwoWayList.h"
#include "ID.h"
#include "ChunkID.h"
#include "QueryID.h"
#include "WayPointID.h"
#include "EfficientMap.h"

/** This header contains Data types useful for the GLA implementation */


/** Base class for GLAStates

    glaType is the MD5 sum of the name. Used for debugging/correctness.
*/
<?php
grokit\create_base_data_type( "GLAState", "DataC", [ 'glaType' => 'uint64_t', ], [ ], false );
?>


// container types for states
typedef TwoWayList <GLAState> GLAStateContainer;
typedef EfficientMap <QueryID, GLAState> QueryToGLAStateMap;
typedef EfficientMap <QueryID, GLAStateContainer> QueryToGLASContMap;

typedef EfficientMap< QueryID, Swapify<int> > QueryIDToInt;
typedef EfficientMap< QueryID, Swapify<bool> > QueryIDToBool;

typedef TwoWayList<WayPointID> ReqStateList;
typedef EfficientMap<QueryID, ReqStateList> QueryToReqStates;

/** GLAPointer stores a pointer to a GLA state. This is an in-memory
    pointer.
*/

<?php
grokit\create_data_type( "GLAPtr", "GLAState", [ 'glaPtr' => 'void*', ], [ ], false );
?>


/******* Data containing QueryIDSet and Chunk **/
<?php
grokit\create_base_data_type( "CacheData", "DataC", [ 'queryIDs' => 'QueryIDSet', ], [ 'cacheChunk' => 'Chunk', ], false );
?>


typedef EfficientMap <ChunkID, CacheData> ChunkToCacheMap;

/** Data Structures for GIST Waypoints */

<?php
grokit\create_base_data_type( "GISTWorkUnit", "DataC", [ ], [ 'gist' => 'GLAState', 'localScheduler' => 'GLAState', 'gla' => 'GLAState', ], false );
?>


typedef TwoWayList <GISTWorkUnit> GistWUContainer;
typedef EfficientMap<QueryID, GISTWorkUnit> QueryToGistWorkUnit;
typedef EfficientMap<QueryID, GistWUContainer> QueryToGistWUContainer;


#endif // GLA_DATA_H
