
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

#ifndef _EXEC_ENGINE_MESSAGES_H_
#define _EXEC_ENGINE_MESSAGES_H_

#include "DataPathGraph.h"
#include "Tokens.h"
#include "WayPointConfigureData.h"
#include "EEMessageTypes.h"
#include "Tasks.h"

// this has all of the message types that can be sent to the execution engine from outside...
// right now there are only three: a configuration message, a data message to send thru
// the system, and a message to just give a token back

// this is the message that is sent to the execution engine to modify/add the graph and/or
// the waypoints that are currently active in the system
<?php
grokit\create_message_type( 'ConfigureExecEngineMessage', [ ], [
    'newGraph' => 'DataPathGraph',
    'configs' => 'WayPointConfigurationList',
    'tasks' => 'TaskList',
    ] );
?>


// this is the message that is sent to the execution engine by a worker to send a hopping
// data message through the graph.  The secod param ("token") is the work token that was used
// to authorize the actual work that was done.  The first param is the return value from the
// function that was called to produce the data...
<?php
grokit\create_message_type( 'HoppingDataMsgMessage', [ 'returnVal' => 'int', ], [ 'token' => 'GenericWorkToken', 'message' => 'HoppingDataMsg', ] );
?>


//////////// QUERIES DONE MESSAGE /////////////

/** When the execution engine has determined that some queries have completed
        execution, it sends the following message to the controller, which lists all
        of the query/exit combos for which work has been completed.

        Arguments:
            completedQueries: the set of completed queries
*/
<?php
grokit\create_message_type( 'QueriesDoneMessage', [ ], [ 'completedQueries' => 'QueryExitContainer', ] );
?>


//////////// TICK MESSAGE /////////////
/** Used to check if there are any requests we can now satisfy */

<?php
grokit\create_message_type( 'TickMessage', [ ], [ ] );
?>


#endif
