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

#include "WayPointImp.h"

#ifndef TABLE_SCAN_IMP_H
#define TABLE_SCAN_IMP_H

class TableScanWayPointImp : public WayPointImp {

private:

	// these are the query-exits we are producing data for
	QueryExitContainer myExits;
	int numQueryExits;

	// this is the number of chunks completed for each query
	int *numDone;

	// this is the bitmap for completed chunks for each query
	int **allDone;

	// the last chunk sent
	int lastOne;

	// the number of read requests outstanding
	int numRequestsOut;

	// whether this one is to the left or right of the join
	int isLHS;

public:

	// constructor and destructor
	TableScanWayPointImp();
	~TableScanWayPointImp();

	// here we over-ride the standard WayPointImp functions
	void TypeSpecificConfigure (WayPointConfigureData &configData);
	void RequestGranted (GenericWorkToken &returnVal);
	void ProcessHoppingUpstreamMsg (HoppingUpstreamMsg &message);
	void ProcessDropMsg (QueryExitContainer &whichOnes, HistoryList &lineage);
	void ProcessAckMsg (QueryExitContainer &whichOnes, HistoryList &lineage);

};

#endif
