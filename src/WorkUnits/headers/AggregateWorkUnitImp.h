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

#ifndef _AGGWORKUNIT_IMP_H_
#define _AGGWORKUNIT_IMP_H_

#include "WorkUnitImp.h"
#include "AggregateWorkUnit.h"
#include "Chunk.h"

class AggregateWorkUnitImp : public WorkUnitImp {

protected:

	struct Info {

		// either PROCESS or GETAGG
		int myType;

		// these point to the actual function that this guy will use
		// the first function processes one chunk, and the second produces output
		void (*process) (Chunk &, AggStorageMap &aggFuncs, pthread_mutex_t *);
		void (*aggregate) (Chunk &, AggStorageMap &aggFuncs,
			QueryExitContainer &addToOutChunk, TableScanID &addMeTo);

		// this stores the state of all of the contained aggregate functions; there
		// is also a mutex that protects the state
		AggStorageMap *aggFuncs;
		pthread_mutex_t *protectAggData;

		// this is used when we are doing the final aggregate comp
		AggStorageMap finalAggs;

		// this is the set of queries that we are finishing up if we are doing a final
		// aggregate
		QueryExitContainer myQueries;

		// this is the table scan ID that is stamped on the output chunk
		TableScanID outID;

		// this is the input chunk
		Chunk input;

		// this is the output chunk that has been created
		Chunk output;
	};

	Info data;

public:

	// constructor and destructor
	AggregateWorkUnitImp ();
	virtual ~AggregateWorkUnitImp ();

	// load up the work unit (see AggregateWorkUnit.h for details)
	void ProcessChunk (void (*processChunk) (Chunk &, AggStorageMap &aggFuncs,
		pthread_mutex_t *), Chunk &processMe, AggStorageMap &aggData,
		pthread_mutex_t *protectAggData);

	void GetAgg (void (*aggFunc) (Chunk &, AggStorageMap &aggFuncs,
		QueryExitContainer &addToOutChunk, TableScanID &addMeTo),
		AggStorageMap &aggData, QueryExitContainer &finishedQueries, TableScanID &tSID);

	// get the output chunk
	int GetOutput (Chunk &output);

	// reclaim the input chunk
	void GetInput (Chunk &input);

	// return the type
	int GetType ();

	// do the work
	void Run ();
};

#endif
