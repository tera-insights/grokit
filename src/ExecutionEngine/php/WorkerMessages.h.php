
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



#include "WorkDescription.h"
#include "ExecEngineData.h"
#include "Tokens.h"
#include "History.h"
#include "Message.h"
#include "WorkFuncs.h"

#ifndef _CPU_WORKER_MESSAGES_H_
#define _CPU_WORKER_MESSAGES_H_

// this is the message sent to a CPU worker to get it to do its job... it includes
// (1) currentPos, the waypoint who is making the request; (2) the function myFunc that
// the CPU worker will be asked to execute; (3) dest, which is the set of query-exit
// pairs where the result of the work will be sent; (4) lineage, which is the history
// that will be sent along with the completed data; (5) token, which is the token that
// enabled someone to request the CPU work in the first place, and (5) workDescription,
// which tells the CPU worker what to actually do

// note that this message is internal in the sense that all CPU workers are managed as
// part of a CPUWorkerPool object; all requests for work should go to that object and
// NOT directly to some CPU worker

<?php
grokit\create_message_type(
    'WorkRequestMsg'
    , [ 'currentPos' => 'WayPointID', 'myFunc' => 'WorkFunc', ]
    , [ 'dest' => 'QueryExitContainer', 'lineage' => 'HistoryList', 'token' => 'GenericWorkToken', 'workDescription' => 'WorkDescription', ]
);
?>


#endif
