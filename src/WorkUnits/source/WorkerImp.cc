//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
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
#include "WorkerImp.h"
#include "Timer.h"
#include "WorkMessages.h"


WorkerImp :: WorkerImp (EventProcessor &parent, WorkerList &putHereWhenDone) {

	// load up the initialization
	executionEngine.swap (parent);
	repository.swap (putHereWhenDone);

	// and register the ProcessWorkUnit method
	RegisterMessageProcessor(WorkToDoMessage::type, &ProcessWorkUnit, 1);
}

MESSAGE_HANDLER_DEFINITION_BEGIN(WorkerImp, ProcessWorkUnit, WorkToDoMessage)
	Timer clock;
	clock.Restart();

	// run the work unit
	msg.workUnit.Run ();

	// and put ourselves into the queue of idle workunits
	// evProc.repository.Add (msg.myName);

	// send back the result
	WorkDoneMessage_Factory (evProc.executionEngine, clock.GetTime(),
		msg.workUnit, msg.myName);

MESSAGE_HANDLER_DEFINITION_END
