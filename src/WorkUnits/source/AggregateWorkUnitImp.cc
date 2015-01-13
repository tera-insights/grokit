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

#include "AggregateWorkUnitImp.h"

AggregateWorkUnitImp :: AggregateWorkUnitImp () {
	data.process = NULL;
	data.aggregate = NULL;
}

AggregateWorkUnitImp :: ~AggregateWorkUnitImp () {
}

void AggregateWorkUnitImp :: ProcessChunk (
	void (*processChunk) (Chunk &, AggStorageMap &aggFuncs, pthread_mutex_t *),
	Chunk &processMe, AggStorageMap &aggData, pthread_mutex_t *protectAggData) {

	data.process = processChunk;
	data.aggFuncs = &aggData;
	data.protectAggData = protectAggData;
	processMe.swap (data.input);
	data.myType = PROCESS;
}

void AggregateWorkUnitImp :: GetAgg (
	void (*aggFunc) (Chunk &, AggStorageMap &aggFuncs, QueryExitContainer &addToOutChunk,
	TableScanID &addMeTo), AggStorageMap &aggData, QueryExitContainer &finishedQueries,
	TableScanID &addMeTo) {

	data.aggregate = aggFunc;
	data.finalAggs.swap (aggData);
	data.myQueries.swap (finishedQueries);
	data.outID.swap (addMeTo);
	data.myType = GETAGG;
}

void AggregateWorkUnitImp :: GetInput (Chunk &input) {

	if (data.myType != PROCESS) {
		cerr << "You can't get an input chunk from the final agg computation.\n";
		exit (1);
	}

	input.swap (data.input);
}

int AggregateWorkUnitImp :: GetOutput (Chunk &output) {

	if (data.myType != GETAGG) {
		cerr << "You can't get an output chunk from a non-final AggregateWorkUnit.\n";
		return -1;
	}

	output.swap (data.output);
	return 0;
}

int AggregateWorkUnitImp :: GetType () {
	return data.myType;
}

void AggregateWorkUnitImp :: Run () {

	if (data.myType == GETAGG) {
		description = "Aggregate FINALIZE";
		(*(data.aggregate)) (data.output, data.finalAggs, data.myQueries, data.outID);
	} else if (data.myType == PROCESS) {
		description = "Aggregate PROCESS";
		(*(data.process)) (data.input, *(data.aggFuncs), data.protectAggData);
	} else {
		cerr << "Got some sort of strage agg work unit type.\n";
		exit (1);
	}
}
