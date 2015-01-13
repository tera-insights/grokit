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
#ifndef WORKER_IMP_H
#define WORKER_IMP_H

#include "EventProcessor.h"
#include "EventProcessorImp.h"
#include "MessageMacros.h"
#include "DistributedQueue.h"
#include "DistributedQueue.cc"

//forward definition
class Worker;

// a list of Worker threads who are ready for some more work
typedef DistributedQueue<Worker> WorkerList;


class WorkerImp : public EventProcessorImp {

	// this is were we put ourselves when we finish
	WorkerList repository;

public:

	// constructor... takes the parent (execution engine) as input
	// as well as the distributed queue where we put ourselves when
	// we are done and ready for some more work
	WorkerImp (EventProcessor &parent, WorkerList &putHereWhenDone);
	WorkerImp ();

	// this is the message handler used to run a work unit
	MESSAGE_HANDLER_DECLARATION(ProcessWorkUnit)

};

#endif
