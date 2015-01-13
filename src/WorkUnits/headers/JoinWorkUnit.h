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

#ifndef _JOINWORKUNIT_H_
#define _JOINWORKUNIT_H_

#include "WorkUnit.h"
#include "Join.h"

// these are the five different join workunit types
#define JOINLHS 1
#define JOINRHS 2
#define FINALMERGE 3
#define FINALMERGE_AND_ALLOCATE 4
#define ALLOCATE 5
#define MERGE 6


/**
	* This is a WorkUnit specialized for the join operation
**/
class JoinWorkUnit : public WorkUnit {

public:

	// tells whether this was the LHS or a RHS join work unit
	int GetType ();

	// generic constructor and destructor
	JoinWorkUnit ();
	virtual ~JoinWorkUnit ();

	// loads up a probe request for one table into the WorkUnit
	void LoadHashProbe (int (*func) (Chunk &, HashTable &, HashTable &, int, Chunk &),
		Chunk &processMe, HashTable &tableOne, int myJoinID);

	// loads up a probe request for two tables into the WorkUnit
	void LoadHashProbe (int (*func) (Chunk &, HashTable &, HashTable &, int, Chunk &),
		Chunk &processMe, HashTable &tableOne, HashTable &tableTwo);

	// loads up a hash table build request for two one table (no merge) into the WorkUnit
	void LoadHashBuild (int (*func) (Chunk &, HashTable &, HashTable &, HT_INDEX_TYPE,
		HT_INDEX_TYPE, HashesToRunList&, ChunkHeaderList&), Chunk &processMe,
		HashTable &tableOne, ChunkHeaderList &completedChunks, int myJoinID);

	// loads up a hash table build request for two tables (with merge) into the WorkUnit
	// DEFUNCT!!
	void LoadHashBuild (int (*func) (Chunk &, HashTable &, HashTable &,
		HT_INDEX_TYPE, HT_INDEX_TYPE, HashesToRunList&, ChunkHeaderList&),
		Chunk &processMe, HashTable &tableOne, HashTable &tableTwo,
		HT_INDEX_TYPE lowToMerge, HT_INDEX_TYPE highToMerge,
		HashesToRunList &extraHashes, ChunkHeaderList &completedChunks);

	// loads up the merge of a set of small, orphaned hash tables
	void LoadFinalMerge (HashTable &mergeIntoMe);

	// just like the above, but also has the job of allocating the hash table
	void LoadFinalMergeAndAllocate (HashTable &mergeIntoMe);

	// just allocate the hash table
	void LoadAllocate (HashTable &mergeIntoMe);

	// returns 1 if the worker was running a merge; 0 otherwise
	int WasMerging ();

	// tells us if there are any little unmerged hash tables
	int AnyExtras ();

	// decremements the read/write counts in the hash tables as needed
	void AdjustReadWriteCounts ();

	// exract the output chunk
	void GetOutput (Chunk &output);

	// extract the input chunk
	void GetInput (Chunk &input);

	// These functins are used to keep track of the number of
	// hash entries that have not yet been added into any slot
	HT_INDEX_TYPE GetSlotsStillOutThere ();
	void ModifyStillOutThere (int delta);

	// creates a work unit to do a merge
	void LoadMerge (HashTable &tableOne, HashTable &tableTwo, HT_INDEX_TYPE low,
		HT_INDEX_TYPE high);

};

#endif
