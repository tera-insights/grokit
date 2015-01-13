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

#include "AggregateWorkUnit.h"
#include "AggregateWorkUnitImp.h"

int AggregateWorkUnit :: GetType () {
	AggregateWorkUnitImp &tmp = dynamic_cast <AggregateWorkUnitImp &> (*data);
	return tmp.GetType ();
}

void AggregateWorkUnit :: GetInput (Chunk &input) {
	AggregateWorkUnitImp &tmp = dynamic_cast <AggregateWorkUnitImp &> (*data);
	tmp.GetInput (input);
}

int AggregateWorkUnit :: GetOutput (Chunk &output) {
	AggregateWorkUnitImp &tmp = dynamic_cast <AggregateWorkUnitImp &> (*data);
	return tmp.GetOutput (output);
}

void AggregateWorkUnit :: ProcessChunk (
	void (*processChunk) (Chunk &, AggStorageMap &aggFuncs, pthread_mutex_t *),
	Chunk &processMe, AggStorageMap &aggData, pthread_mutex_t *protectAggData) {

	AggregateWorkUnitImp &tmp = dynamic_cast <AggregateWorkUnitImp &> (*data);
	tmp.ProcessChunk (processChunk, processMe, aggData, protectAggData);
}

void AggregateWorkUnit :: GetAgg (
	void (*aggFunc) (Chunk &, AggStorageMap &aggFuncs, QueryExitContainer &, TableScanID &),
	AggStorageMap &aggData, QueryExitContainer &finishedQueries, TableScanID &myID) {

	AggregateWorkUnitImp &tmp = dynamic_cast <AggregateWorkUnitImp &> (*data);
	tmp.GetAgg (aggFunc, aggData, finishedQueries, myID);
}

AggregateWorkUnit :: AggregateWorkUnit () {
	delete data;
	data = new AggregateWorkUnitImp;
}

AggregateWorkUnit :: ~AggregateWorkUnit () {
	delete data;
	data = NULL;
}
