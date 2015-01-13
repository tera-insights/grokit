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

#ifndef _JOINWORKUNIT_IMP_H_
#define _JOINWORKUNIT_IMP_H_

#include "WorkUnitImp.h"
#include "JoinWorkUnit.h"
#include "Chunk.h"

#define JOINLHS 1
#define JOINRHS 2
#define FINALMERGE 3


/**
	* This is a WorkUnit specialized for the join operation
*/
class JoinWorkUnitImp : public WorkUnitImp {

protected:

	struct Info {

		// these are the two functions that actually do the work
		int (*ProbePtr) (Chunk &, HashTable &, HashTable &, int, Chunk &);
		int (*HashPtr) (Chunk &, HashTable &, HashTable &, HT_INDEX_TYPE,
			HT_INDEX_TYPE, HashesToRunList&, ChunkHeaderList&);

		// either JOINLHS or JOINRHS
		int myType;

		// the input and output chunks;
		Chunk inChunk;
		Chunk outChunk;

		// the two hash tables to work on
		HashTable *smallTable;
		HashTable *largeTable;

		// in the case of a RHS, this is the range of values to merge
		HT_INDEX_TYPE lowMerge;
		HT_INDEX_TYPE highMerge;

		// this is the set of chunks that we are done with
		ChunkHeaderList completedChunks;

		// this is the set of little hashes that still need to be added to the current hash table
		HashesToRunList extraHashes;

		// which join wants the work
		int myJoinID;
	};

	Info data;

public:

	// tells whether this was the LHS or a RHS join work unit
	int GetType ();

	// constructor
	JoinWorkUnitImp ();

	// destructor
	virtual ~JoinWorkUnitImp () {}

	// actually run the work that this guy needs to do
	void Run();

	// this one tells the work unit to convert itself into a type that will try
	// to add all of its little hash tables into the corresponding big hash table
	void LoadFinalMerge (HashTable &mergeIntoMe);

	// just like the above, but it has the job of allocating the hash as well
	void LoadFinalMergeAndAllocate (HashTable &mergeIntoMe);

	// just allocate the hash
	void LoadAllocate (HashTable &mergeIntoMe);

	// loads up a probe request for one table into the WorkUnit
	void LoadHashProbe (int (*func) (Chunk &, HashTable &, HashTable &, int, Chunk &),
		Chunk &processMe, HashTable &tableOne, int myJoinID);

	// loads up a probe request for two tables into the WorkUnit
	void LoadHashProbe (int (*func) (Chunk &, HashTable &, HashTable &, int, Chunk &),
		Chunk &processMe, HashTable &tableOne, HashTable &tableTwo);

	// loads up a hash table build request for two one table (no merge) into the WorkUnit
	void LoadHashBuild (int (*func) (Chunk &, HashTable &, HashTable &, HT_INDEX_TYPE,
		HT_INDEX_TYPE, HashesToRunList&, ChunkHeaderList&),
		Chunk &processMe, HashTable &tableOne, ChunkHeaderList &chunks, int myJoinID);

	// loads up a hash table build request for two tables (with merge) into the WorkUnit
	void LoadHashBuild (int (*func) (Chunk &, HashTable &, HashTable &, HT_INDEX_TYPE,
		HT_INDEX_TYPE, HashesToRunList&, ChunkHeaderList&),
		Chunk &processMe, HashTable &tableOne, HashTable &tableTwo,
		HT_INDEX_TYPE lowToMerge, HT_INDEX_TYPE highToMerge,
		HashesToRunList &hashes, ChunkHeaderList &chunks);

	// returns 1 if the worker was running a merge; 0 otherwise
	int WasMerging ();

	// decremements the read/write counts in the hash tables as needed
	void AdjustReadWriteCounts ();

	// exract the output chunk
	void GetOutput (Chunk &output);

	// extract the input chunk
	void GetInput (Chunk &input);

	// tells us if there are any little hash tables to proces
	int AnyExtras ();

	// These functins are used to keep track of the number of
	// hash entries that have not yet been added into any slot
	HT_INDEX_TYPE GetSlotsStillOutThere ();
	void ModifyStillOutThere (int delta);

	// creates a work unit to run a merge
	void LoadMerge (HashTable &tableOne, HashTable &tableTwo,
		HT_INDEX_TYPE low, HT_INDEX_TYPE high);
};

#endif
