
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



#ifndef WAY_POINT_CONFIG_H
#define WAY_POINT_CONFIG_H

#include "TwoWayList.h"
#include "ID.h"
#include "WorkFuncs.h"
#include "ContainerTypes.h"
#include "GLAData.h"
#include "SerializeJson.h"

#include <string>
#include <map>
#include <utility>

// this contains all of the info needed to configure a waypoint

// this is the basic configuration data type that is sent into a waypoint... myID is the waypoint's identifier;
// funcList is a list of functions to add to the waypoint (if another function with the same wrapper is in the
// waypoint, it is overwritten), endingQueryExits is the set of query exits that go no further than the waypint,
// and flowThroughQueryExits is the set of query exits whose data goes through the waypoint on its way to somewhere
// else in the system
<?php
grokit\create_base_data_type(
    "WayPointConfigureData"
    , "Data"
    , [ 'myID' => 'WayPointID', ]
    , [ 'funcList' => 'WorkFuncContainer', 'endingQueryExits' => 'QueryExitContainer', 'flowThroughQueryExits' => 'QueryExitContainer', ]
    , true
    , true
);
?>


// we should have one of these for each of the types of waypoints in the system...
// this contains all of the additional data that is needed to configure the waypoint,
// above and beyond the data that is held in the basic WayPointConfigureData class.
<?php
grokit\create_data_type(
    "AggregateConfigureData"
    , "WayPointConfigureData"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "WriterConfigureData"
    , "WayPointConfigureData"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "JoinMergeConfigureData"
    , "WayPointConfigureData"
    , [ ]
    , [ ]
    , true
);
?>

<?
grokit\create_data_type(
    "CacheChunkConfigureData"
    , "WayPointConfigureData"
    , [ ]
    , [ ]
    , true
);
?>

<?
grokit\create_data_type(
    "CompactChunkConfigureData"
    , "WayPointConfigureData"
    , [ ]
    , [ ]
    , true
);
?>

<?
grokit\create_data_type(
    "ClusterConfigureData"
    , "WayPointConfigureData"
    , [ 'relation' => 'std::string' ]
    , [ ]
    , true
);
?>

// General Processing WayPoint configuration data.
<?php
grokit\create_base_data_type(
    "GPWConfigureData"
    , "WayPointConfigureData"
    , [ ]
    , [ 'reqStates' => 'QueryToReqStates', ]
    , true
);
?>


<?php
grokit\create_data_type(
    "SelectionConfigureData"
    , "GPWConfigureData"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "GLAConfigureData"
    , "GPWConfigureData"
    , [  ]
    , [ 'resultIsState' => 'QueryIDToBool', ]
    , true
);
?>

<?php
grokit\create_data_type(
    "GTConfigureData"
    , "GPWConfigureData"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "GISTConfigureData"
    , "GPWConfigureData"
    , [ ]
    , [ 'resultIsState' => 'QueryIDToBool', ]
    , true
);
?>

using ServiceToQuery = std::map<std::string, QueryID>;

<?php
grokit\create_data_type(
    "GSEConfigureData"
    , "WayPointConfigureData"
    , [ ]
    , [ 'services' => 'ServiceToQuery' ]
    , true
);
?>


typedef TwoWayList< TwoWayList<std::string> > PrintHeader;

<?
grokit\create_data_type(
    'PrintFileInfo'
    , 'Data'
    , [ 'file' => 'std::string', 'separator' => 'std::string', 'type' => 'std::string', 'limit' => 'int' ]
    , [ 'header' => 'PrintHeader' ]
    , true
);
?>

typedef EfficientMap< QueryID, PrintFileInfo> QueryToFileInfoMap;

<?php
grokit\create_data_type(
    "PrintConfigureData"
    , "WayPointConfigureData"
    , [ ]
    , [ 'queriesInfo' => 'QueryToFileInfoMap', ]
    , true
);
?>


// this contains a copy of the central hash table
<?php
grokit\create_data_type(
    "HashTableCleanerConfigureData"
    , "WayPointConfigureData"
    , [ ]
    , [ ]
    , true
);
?>


// this contains a copy of the central hash table, as well as the "disk based twin" waypoint that will be used
// if this waypoint gets kicked out of the hash table.  It also contains a list of the new queries that have
// never been seen before by this join waypoint, since these are treated differently than the existing queries
// in that some state must be recorded for them.  Note that newEndingQueries must be a subset of endingQueryExits,
// and newFlorThruQueries must be a subset of flowThroughQueryExits
<?php
grokit\create_data_type(
    "JoinConfigureData"
    , "WayPointConfigureData"
    , [ 'myDiskBasedTwinID' => 'WayPointID', 'hashTableCleaner' => 'WayPointID', ]
    , [ 'newEndingQueries' => 'QueryExitContainer', 'newFlowThruQueries' => 'QueryExitContainer']
    , true
);
?>


// for my toy table scan waypoint, we need to supply the query exits to send data to, and whethere this is supplying
// data to the LHS or the RHS of the single join in the plan
<?php
grokit\create_data_type(
    "TableScanConfigureData"
    , "WayPointConfigureData"
    , [ 'isLHS' => 'int', ]
    , [ 'myExits' => 'QueryExitContainer', ]
    , true
);
?>


/** real table scanner/writter.

        Arguments:
          relName: relation name
            deletedQE: the queries that finished
            newQE: the query-exitsWP for which we have to generate chunks
            queryColumnsMap: the mapping from query-exitWP to slots they need
            columnsToSlotsMap: the mapping from physical columns to slots
            storeColumnsToSlotsMap: the mapping from physical columns to slots for writing
*/

typedef std::pair<int64_t, int64_t> ScannerRange;
typedef std::vector<ScannerRange> ScannerRangeList;
typedef std::map<QueryID, ScannerRangeList> QueryToScannerRangeList;

<?php
grokit\create_data_type(
    "TableConfigureData"
    , "WayPointConfigureData"
    , [ 'relName' => 'std::string', ]
    , [ 'newQE' => 'QueryExitContainer',
        'deletedQE' => 'QueryExitContainer', 
        'queryColumnsMap' => 'QueryExitToSlotsMap',
        'columnsToSlotsMap' => 'SlotToSlotMap',
        'storeColumnsToSlotsMap' => 'SlotToSlotMap',
        'clusterAttribute' => 'SlotID',
        'filterRanges' => 'QueryToScannerRangeList',]
    , true
);
?>



/** Text Loader

        Arguments:
                files: names of files that we are bulkloading
                queries: the queries that are going to tag the produced chunks
*/
<?php
grokit\create_data_type(
    "TextLoaderConfigureData"
    , "WayPointConfigureData"
    , [ 'files' => 'StringContainer', ]
    , [ 'queries' => 'QueryExitContainer', ]
    , true
);
?>


<?php
grokit\create_data_type(
    "TileJoinConfigureData"
    , "WayPointConfigureData"
    , [ 'lhsrelName' => 'std::string', 'rhsrelName' => 'std::string', ]
    , [ 'JoinWP' => 'WayPointID', 'queryColumnsMapLhs' => 'QueryExitToSlotsMap', 'queryColumnsMapRhs' => 'QueryExitToSlotsMap', 'columnsToSlotsPairLhs' => 'SlotPairContainer', 'columnsToSlotsPairRhs' => 'SlotPairContainer', ]
    , true
);
?>


<?php
grokit\create_data_type(
    "GIConfigureData"
    , "WayPointConfigureData"
    , [ 'files' => 'StringContainer', ]
    , [ 'queries' => 'QueryExitContainer', ]
    , true
);
?>

<?
grokit\generate_deserializer( 'WayPointConfigureData' );
?>

// this is the list of way point configurations sent into the execution engine
typedef TwoWayList<WayPointConfigureData> WayPointConfigurationList;

#endif
