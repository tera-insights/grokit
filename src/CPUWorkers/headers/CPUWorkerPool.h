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

#ifndef CPU_WORKER_POOL_H
#define CPU_WORKER_POOL_H

#include "CPUWorker.h"
#include "CPUWorkerImp.h"
#include "ID.h"
#include "History.h"
#include "Tokens.h"
#include "WorkDescription.h"
#include "WorkFuncs.h"

class CPUWorkerPool {

private:

	CPUWorkerList myWorkers;

public:

	// set up all of the worker threads and puts them into the list myWorkers
	CPUWorkerPool (int numWorkers);

	// die
	virtual ~CPUWorkerPool ();

	// ask one of the workers to do some work.  The work request has the following params.
	// "requestor" is the identifier of the waypoint who is asking for the work.  "lineage"
	// is the lineage info that will be put into the result of the work, and also sent back
	// to the requestor when the work has actually been completed so that the requestor can
	// track the work's progress.  "dest" is the set of query exits that the result of the
	// work should be sent to.  "myToken" is the token object that gave the requestor the
	// right to ask for the work.  "workDescription" is an object that contains all of the
	// data that will be used to perform the work.  Finally, "WorkFunc" is a pointer to the
	// actual function that will be called by the worker (using "workDescription" as a param)
	// to actually do the work that is being requested.  Note that when the work is done,
	// the result of the work will be put into the param "result" by the function WorkFunc.
	// Then, DoSomeWork will take the resulting ExecEngineData object, and send it back to
	// the execution engine along with the lineage, the token, and the destination(s).
	void DoSomeWork (WayPointID &requestor, HistoryList &lineage, QueryExitContainer &dest,
		GenericWorkToken &myToken, WorkDescription &workDescription, WorkFunc &myFunc);

	// add a worker back into the pool
	void AddWorker (CPUWorker &addMe);

	// returns the number of available threads
	int NumAvailable(void){ return myWorkers.Length(); }
};

// myWorkers actually lives in CPUWorkerPool.cc
extern CPUWorkerPool myCPUWorkers;
extern CPUWorkerPool myDiskWorkers;


#endif
