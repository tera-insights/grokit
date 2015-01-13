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
// for Emacs -*- c++ -*-

#ifndef _AGGWORKUNIT_H_
#define _AGGWORKUNIT_H_

#include "WorkUnit.h"
#include "Chunk.h"
#include "AggStorage.h"

#define PROCESS 1
#define GETAGG 2

/**
	* This is a WorkUnit specialized for the join operation
**/

class AggregateWorkUnit : public WorkUnit {

public:

	// constructor and destructor
	AggregateWorkUnit ();
	virtual ~AggregateWorkUnit ();

	// loads up a request to process a chunk into the WorkUnit... since multiple
	// threads may work on the same AggStorageMap at the same time, it should be
	// protected with the mutex pointed to by protectAggData (I know... I'm using
	// a pointer here, but the only place that this function is ever called is by
	// the aggregate waypoint!)
	void ProcessChunk (void (*processChunk) (Chunk &, AggStorageMap &aggFuncs,
		pthread_mutex_t *protectAggData), Chunk &processMe,
		AggStorageMap &aggData, pthread_mutex_t *protectAggData);

	// loads up a request to finish up the specified aggregate functions, and put
	// them into an output chunk... the first param is the func that builds the
	// output chunk.  The 2nd is the data for all of the aggregate functions that
	// need to be finished... the 3rd is the metadata that tells the output chunk
	// where it needs to go, and the 4th is the ID of the table scan that produced
	// this guy
	void GetAgg (void (*aggFunc) (Chunk &, AggStorageMap &aggFuncs,
		QueryExitContainer &addToOutChunk, TableScanID &addMeTo),
		AggStorageMap &aggData, QueryExitContainer &finishedQueries, TableScanID &tSID);

	// exract the output chunk (returns -1 iff there is no chunk to get)
	int GetOutput (Chunk &output);

	// extract the input chunk
	void GetInput (Chunk &input);

	// returns the type of this workunit
	int GetType ();
};

#endif
