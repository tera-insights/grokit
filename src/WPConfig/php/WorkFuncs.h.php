
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



#ifndef WORK_FUNC_H
#define WORK_FUNC_H

// this is the function header that all of the functions that do actual work in
// the system will use.  The first arg is the description of the work that needs
// to be done, and the second arg is the place where the function is supposed to
// put its result... returns 1 iff a result is actually produced

// If DUMMY_WORK_FUNC is defined, WorkFunc is just an int.
// This allows us to use the same hierarchy in both the brain and body without
// including GLAData in the brain.
#ifdef DUMMY_WORK_FUNC
typedef int WorkFunc;
#else
#include "GLAData.h"
#include "ExecEngineData.h"
#include "WorkDescription.h"
typedef int (* WorkFunc) (WorkDescription &workDescription, ExecEngineData &result);
#endif

inline
void ToJson( const WorkFunc & src, Json::Value & dest ) {
    dest = Json::Value((Json::Int64) src);
}

inline
void FromJson( const Json::Value & src, WorkFunc & dest ) {
    dest = (WorkFunc) src.asInt64();
}

// this contains all of the work func types

// this is the basic wrapper for the work functions that are sent into waypoints
// at configuration time.  All functions to be run are of type "WorkFunc";
// see the file "WorkerMessages.h.m4" in CPUWorkers/m4 for the definition of "WorkFunc"
<?php
grokit\create_base_data_type(
    "WorkFuncWrapper"
    , "DataC"
    , [ 'myFunc' => 'WorkFunc', ]
    , [ ]
    , true
    , true
);
?>


// this is a list of work functions that are used to configure a waypoint
typedef TwoWayList <WorkFuncWrapper> WorkFuncContainer;

// We should have one of these for each of the types of work funcs in the system.
// It does not have anything in it above and beyond what the WorkFuncWrapper has.
// but the fact that we have one of these for each of the work functions means that
// we can put the work functions in a list of swappable objects, and do things like
// search the list for the "AggFinishUpWorkFunc" function, or the "AggWorkFunc"
// function
//Join
<?php
grokit\create_data_type(
    "JoinLHSWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "JoinRHSWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "JoinLHSHashWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "JoinMergeWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "CleanerWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "WriterWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

//Aggregate
<?php
grokit\create_data_type(
    "AggOneChunkWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "AggFinishUpWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

// Selection
<?php
grokit\create_data_type(
    "SelectionPreProcessWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "SelectionProcessChunkWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

// Print
<?php
grokit\create_data_type(
    "PrintWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?
grokit\create_data_type(
    'PrintFinalizeWorkFunc'
    , 'WorkFuncWrapper'
    , [ ]
    , [ ]
    , true
);
?>

// TableScan
<?php
grokit\create_data_type(
    "TableScanWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

// TextLoader
<?php
grokit\create_data_type(
    "TextLoaderWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "GIProduceChunkWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>


/** WorkFuncs for GLA*/
<?php
grokit\create_data_type(
    "GLAPreProcessWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "GLAProcessChunkWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "GLAPreFinalizeWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "GLAFinalizeWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "GLAFinalizeStateWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "GLAMergeStatesWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "GLAPostFinalizeWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>


/** WorkFuncs for GTs */
<?php
grokit\create_data_type(
    "GTPreProcessWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "GTProcessChunkWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>


/** WorkFuncs for GISTs */

<?php
grokit\create_data_type(
    "GISTPreProcessWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "GISTNewRoundWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "GISTDoStepsWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "GISTMergeStatesWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "GISTShouldIterateWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "GISTProduceResultsWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?php
grokit\create_data_type(
    "GISTProduceStateWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?
grokit\create_data_type(
    "CacheChunkWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?
grokit\create_data_type(
    "GSEPreProcessWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?
grokit\create_data_type(
    "GSEProcessReadOnlyWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
);
?>

<?
grokit\create_data_type(
    "ClusterProcessChunkWorkFunc"
    , "WorkFuncWrapper"
    , [ ]
    , [ ]
    , true
); 
?>

<?
grokit\generate_deserializer( 'WorkFuncWrapper' );
?>

#endif
