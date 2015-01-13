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

#ifndef AGG_WAY_POINT_IMP
#define AGG_WAY_POINT_IMP

#include "ID.h"
#include "History.h"
#include "Tokens.h"
#include "WayPointImp.h"
#include "AggStorageMap.h"

/** WARNING: The chunk processing functin has to return 0 and the
		finalize function 1 otherwise acknoledgements are not sent
		properly in the system
*/
class AggWayPointImp : public WayPointImp {

	// this is the set of queries for which we know we have received all of the data, and so we
	// are just waiting for a worker to be available so that we can actually finish them up
	QueryExitContainer queriesToComplete;
	
	QueryIDSet queriesCompleted;


	// this is the set of aggregate storage objects that we are working on at this time
	AggStorageMap myAggs;

	void PrintMyAggs(void);

public:

	// constr. and destr.
	virtual ~AggWayPointImp ();
	AggWayPointImp ();

	// these are just implementations of the standard WayPointImp functions
	void DoneProducing (QueryExitContainer &whichOnes, HistoryList &history, int returnVal, ExecEngineData& data);
	void RequestGranted (GenericWorkToken &returnVal);
	void ProcessHoppingDataMsg (HoppingDataMsg &data);
	void ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message);
	void ProcessAckMsg (QueryExitContainer &whichOnes, HistoryList &lineage);
	void ProcessDropMsg (QueryExitContainer &whichOnes, HistoryList &lineage);
	
};

#endif
