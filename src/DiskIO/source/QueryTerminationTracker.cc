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

#include "QueryTerminationTracker.h"
#include "Errors.h"

#include <iostream>
#include <limits>

QueryTerminationTracker :: QueryTerminationTracker() {
	counter = UINT_MAX ;
}

QueryTerminationTracker :: QueryTerminationTracker(unsigned int noChunks) {
	counter = noChunks;
}

// destructor
QueryTerminationTracker :: ~QueryTerminationTracker(void){
	if (counter>0 && counter != UINT_MAX){
		FATAL("Removing a query that did not finish");
	}
};

bool QueryTerminationTracker :: IsValid(void){ return counter != -1; }

// return true if the the query finished
bool QueryTerminationTracker :: ProcessChunk(ChunkID &cID){
	// we do not care what chunk we see since they are all different
	counter--;
// 	cerr << "**" << counter << "**";
	return (counter<=0);
}

void QueryTerminationTracker :: swap (QueryTerminationTracker &withMe) {
	unsigned int temp = counter;
	counter = withMe.counter;
	withMe.counter = temp;
}
