/**
	This header file contains the messages used by Workers.
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



#ifndef _WORK_MESSAGES_H_
#define _WORK_MESSAGES_H_

#include "MessageMacros.h"
#include "Message.h"
#include "Worker.h"
#include "WorkUnit.h"


//////////// WORK TO DO MESSAGE /////////////

/** Message sent by ExecutionEngine to request work to be done by a Worker.

		Arguments:
			workUnit: work to be done
			myName: handle for the Worker
*/
<?php
grokit\create_message_type( 'WorkToDoMessage', [ ], [ 'workUnit' => 'WorkUnit', 'myName' => 'Worker', ] );
?>



//////////// WORK DONE MESSAGE /////////////

/** Message sent by Worker to the ExecutionEngine when the work is done.

		Arguments:
			timeS: how long did it take to execute the work
			runThis: work that has been done
			sentFrom: Worker who did the work and sent the message
*/
<?php
grokit\create_message_type( 'WorkDoneMessage', [ 'timeS' => 'double', ], [ 'runThis' => 'WorkUnit', 'sentFrom' => 'Worker', ] );
?>


#endif // _WORK_MESSAGES_H_
