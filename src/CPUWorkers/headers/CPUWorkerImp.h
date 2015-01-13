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

#ifndef CPU_WORKER_H
#define CPU_WORKER_H

#include "EventProcessor.h"
#include "EventProcessorImp.h"
#include "DistributedQueue.h"
#include "DistributedQueue.cc"
#include "EventProcessor.h"
#include "Message.h"
#include "PerfCounter.h" // perfrormance counters
#include "Timer.h"

class CPUWorker;

// a list of CPU worker threads who are ready for some more work
typedef DistributedQueue <CPUWorker> CPUWorkerList;

// this data type controls a thread that can be assigned work to do by
// a waypoint that has a computational task... the thread is asked to
// do some work via a call to the message handler "DoSomeWork".  Note
// that no one should communicate with an object of this type directly.
// Instead, all requests for CPU work are made via the CPUWorkerPool
// class, which maintains the canonical list of CPUWorkerImp objects
//
class CPUWorkerImp : public EventProcessorImp {

private:

	EventProcessor me;

	/* Performance counters to watch what the functions being executed are doing */
	/* For now the info is just logged but it could be sent the the reciever in a
	   special package for self-diagnosis */
	Timer clock; /* for real walclock time */

	/* cached values of the counters for convenience */
	double clock_C; // time in seconds


protected:

	// this is called by a CPUWorker so that the CPUWorkerImp can know his/her parent
	friend class CPUWorker;
	void GetCopyOf (EventProcessor &myParent);

public:

	// constructor and destructor
	CPUWorkerImp ();
	~CPUWorkerImp ();

	// this handles a request to actually do some work
	MESSAGE_HANDLER_DECLARATION(DoSomeWork);

};

/////////// INLINE METHODS ////////////

#endif
