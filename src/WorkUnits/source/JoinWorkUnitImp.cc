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

#include "JoinWorkUnitImp.h"
#include "QueryManager.h"

#include <string.h>
#include <iomanip>

using namespace std;


// these are used to keep track of how many outstanding slots of data there are
// that need to be merged into a home hash table
HT_INDEX_TYPE stillOutThere;
pthread_mutex_t myMutexJoin = PTHREAD_MUTEX_INITIALIZER;

void JoinWorkUnitImp :: GetInput (Chunk &putItHere) {
	putItHere.swap (data.inChunk);
}

void JoinWorkUnitImp :: GetOutput (Chunk &putItHere) {
	putItHere.swap (data.outChunk);
}

JoinWorkUnitImp :: JoinWorkUnitImp () {
	data.ProbePtr = NULL;
	data.HashPtr = NULL;
}

HT_INDEX_TYPE JoinWorkUnitImp :: GetSlotsStillOutThere () {
	return stillOutThere;
}

void JoinWorkUnitImp :: ModifyStillOutThere (int delta) {
	pthread_mutex_lock (&myMutexJoin);
	stillOutThere += delta;
	pthread_mutex_unlock (&myMutexJoin);
}

void JoinWorkUnitImp :: LoadHashProbe (int (*func) (Chunk &, HashTable &,
	HashTable &, int, Chunk &), Chunk &processMe, HashTable &tableOne, int myJoinID) {

	data.myType = JOINLHS;
	data.ProbePtr = func;
	data.inChunk.swap (processMe);
	data.smallTable = &tableOne;
	data.largeTable = NULL;
	data.myJoinID = myJoinID;

}

void JoinWorkUnitImp :: LoadFinalMergeAndAllocate (HashTable &mergeIntoMe) {

	LoadFinalMerge (mergeIntoMe);
	data.myType = FINALMERGE_AND_ALLOCATE;
}


void JoinWorkUnitImp :: LoadAllocate (HashTable &mergeIntoMe) {

	// and remember all of the info
	data.myType = ALLOCATE;
	data.largeTable = &mergeIntoMe;
	data.smallTable = NULL;
}

void JoinWorkUnitImp :: LoadFinalMerge (HashTable &mergeIntoMe) {

	// and remember all of the info
	data.myType = FINALMERGE;
	data.largeTable = &mergeIntoMe;
	data.smallTable = NULL;
}

void JoinWorkUnitImp :: LoadHashProbe (int (*func) (Chunk &, HashTable &,
	HashTable &, int, Chunk &), Chunk &processMe, HashTable &tableOne,
	HashTable &tableTwo) {

	data.myType = JOINLHS;
	data.ProbePtr = func;
	data.inChunk.swap (processMe);
	data.smallTable = &tableOne;
	data.largeTable = &tableTwo;
}

void JoinWorkUnitImp :: LoadHashBuild (int (*func) (Chunk &, HashTable &,
	HashTable &, HT_INDEX_TYPE, HT_INDEX_TYPE, HashesToRunList &, ChunkHeaderList &),
	Chunk &processMe, HashTable &tableOne, ChunkHeaderList &completedChunks, int myJoinID) {

	data.myType = JOINRHS;
	data.HashPtr = func;
	data.inChunk.swap (processMe);
	data.smallTable = &tableOne;
	data.largeTable = NULL;
	data.completedChunks.Clone (completedChunks);
	data.myJoinID = myJoinID;
}

int JoinWorkUnitImp :: AnyExtras () {

	return data.extraHashes.Length () != 0;

	//return ((data.myType == JOINRHS || data.myType == FINALMERGE_AND_ALLOCATE
	//|| data.myType == FINALMERGE || data.myType == MERGE) && data.extraHashes.Length ());
}

void JoinWorkUnitImp :: LoadHashBuild (int (*func) (Chunk &, HashTable &,
	HashTable &, HT_INDEX_TYPE, HT_INDEX_TYPE, HashesToRunList&, ChunkHeaderList&),
	Chunk &processMe, HashTable &tableOne, HashTable &tableTwo,
	HT_INDEX_TYPE lowToMerge, HT_INDEX_TYPE highToMerge, HashesToRunList &extraHashes,
	ChunkHeaderList &completedChunks) {

	data.myType = JOINRHS;
	data.HashPtr = func;
	data.inChunk.swap (processMe);
	data.smallTable = &tableOne;
	data.largeTable = &tableTwo;
	data.lowMerge = lowToMerge;
	data.highMerge = highToMerge;
	data.completedChunks.Clone (completedChunks);
	data.extraHashes.Clone (extraHashes);
}

void JoinWorkUnitImp :: LoadMerge (HashTable &tableOne, HashTable &tableTwo,
	HT_INDEX_TYPE low, HT_INDEX_TYPE high) {
	data.myType = MERGE;
	data.smallTable = &tableOne;
	data.largeTable = &tableTwo;
	data.lowMerge = low;
	data.highMerge = high;
}

int JoinWorkUnitImp :: WasMerging () {

	if (data.myType == JOINRHS && data.largeTable != NULL) {
		return 1;
	} else {
		return 0;
	}
}

int JoinWorkUnitImp :: GetType () {
	return data.myType;
}

void JoinWorkUnitImp :: Run () {
	description = "Join DEFUNCT";
	int retVal;

	ostringstream out;

	QueryManager& qm=QueryManager::GetQueryManager();
	QueryIDSet queriesToRun = data.inChunk.GetQueries();

	out << "|";

  while (!queriesToRun.IsEmpty()){
		QueryID x = queriesToRun.GetFirst ();
		string tmp;
		qm.GetQueryName(x, tmp);
		out <<  tmp << " ";
	}

	out << "|";


	if (data.myType == FINALMERGE_AND_ALLOCATE) {

		// first, actually create the table and init it
		cerr << "DEFUNCT!\n\n";
		exit (1);

	} else if (data.myType == ALLOCATE) {

		cerr << "DEFUNCT!\n\n";
		exit (1);

	} else if (data.myType == MERGE) {

		cerr << "DEFUNCT!\n\n";
		exit (1);

	} else if (data.myType == FINALMERGE) {

		cerr << "DEFUNCT!\n\n";
		exit (1);

	} else if (data.myType == JOINLHS) {
		description = "Join LHS";

		// need to probe both hash tables
		retVal = (*(data.ProbePtr)) (data.inChunk, *(data.smallTable),
			*(data.smallTable), data.myJoinID, data.outChunk);

	} else if (data.myType == JOINRHS) {
		description = "Join RHS";

		// just doing a build of the small table
		retVal = (*(data.HashPtr)) (data.inChunk, *(data.smallTable),
			*(data.smallTable), data.myJoinID, -1, data.extraHashes, data.completedChunks);
	}

	if(data.smallTable!=NULL){
		// add the amout of SmallHash used
		out << " SH: " << setprecision(5) << data.smallTable->PercentageUsed()*100 <<
			"% : " << retVal << " ";
	}

	if(data.largeTable!=NULL){
		// add the amout of SmallHash used
		out << " LH: " << setprecision(5) << data.largeTable->PercentageUsed()*100 <<
			"% : " << retVal << " ";
	}

	description+=out.str();

}
