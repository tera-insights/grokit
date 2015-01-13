
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



#ifndef EE_MSG_H
#define EE_MSG_H

#include "Data.h"
#include "ID.h"
#include "ExecEngineData.h"
#include "History.h"
#include "Notifications.h"

// note that in all of these messages, "currentPos" should (usually) be set to be the
// identifier of the waypoint who is sending the message, since the execution engine
// delivers the message to the next logical recipient in the graph after currentPos

// this macro defines the "hopping data message", which is used to send data 
// through the network from a data producer. dest is all of the places that the msg 
// needs to go, lineage is a list of History objects (where each waypoint that
// obtains and then forwards on the message can add new history objects to the
// list), and data is the actual data in the message.
<?php
grokit\create_base_data_type( "HoppingDataMsg", "DataC", [ 'currentPos' => 'WayPointID', ], [ 'dest' => 'QueryExitContainer', 'lineage' => 'HistoryList', 'data' => 'ExecEngineData', ] );
?>


// this macro defines direct messages, that are sent to a particular waypoint
<?php
grokit\create_base_data_type( "DirectMsg", "Data", [ 'receiver' => 'WayPointID', ], [ 'message' => 'Notification', ] );
?>


// this macro defines hopping downstream messages... these are a lot like hopping
// data messages, except that they contain Notification objects instead of data 
<?php
grokit\create_data_type( "HoppingDownstreamMsg", "DataC", [ 'currentPos' => 'WayPointID', ], [ 'dest' => 'QueryExitContainer', 'msg' => 'Notification', ] );
?>


// this macro defines hopping upstream messages... unlike the downstream messages,
// these have only one destination---the waypoint(s) that produce(s) data associated
// with a particular query exit
<?php
grokit\create_data_type( "HoppingUpstreamMsg", "DataC", [ 'currentPos' => 'WayPointID', ], [ 'dest' => 'QueryExit', 'msg' => 'Notification', ] );
?>


#endif
