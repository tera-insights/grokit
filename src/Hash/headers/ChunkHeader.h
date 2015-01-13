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
#ifndef _CHUNKHEADER_H_
#define _CHUNKHEADER_H_

#include "HashEntry.h"
#include "MmapAllocator.h"
#include "Chunk.h"
#include "QueryExit.h"
#include "ID.h"
#include "DistributedQueue.h"
#include "DistributedQueue.cc"


// this structure is used to temporarily store a mini hash table for later use
struct HashHolder {

	HashEntry *entries;

	HashHolder () {
		entries = NULL;
	}

	HashHolder (HashEntry *&entriesIn) {
		entries = entriesIn;
		entriesIn = NULL;
	}

	~HashHolder () {
		if (entries != NULL)
			mmap_free((void*)entries);
		entries = NULL;
	}

	void Extract (HashEntry *&entriesIn) {
		entriesIn = entries;
		entries = NULL;
	}

	void swap (HashHolder &withMe) {
		char temp[sizeof(HashHolder)];
		memmove (temp, this, sizeof(HashHolder));
		memmove (this, &withMe, sizeof(HashHolder));
		memmove (&withMe, temp, sizeof(HashHolder));
	}
};

////////////////////////////////////////////////////////////////////////////////
// this is used to store the header info for a chunk
struct ChunkHeader {

	QueryExitContainer queryExits;
	ChunkID chunkID;

	ChunkHeader () {}

	~ChunkHeader () {}

	void swap (ChunkHeader &withMe) {
		queryExits.swap (withMe.queryExits);
		chunkID.swap (withMe.chunkID);
	}

	void Load (Chunk &input) {
		input.GetQueries (queryExits);
		chunkID = input.GetChunkId ();
	}
};

// set of chunk headers
typedef DistributedQueue<ChunkHeader> ChunkHeaderList;


////////////////////////////////////////////////////////////////////////////////
// this is used by the join to bundle up all of the hashed data from a chunk
// that has not yet been added into a permanent hash table, because the hash
// table was too small
struct UncompletedHash {

	ChunkHeader source;
	TwoWayList <HashHolder> hashes;
	int empty;

	UncompletedHash () {
		empty = true;
	}

	~UncompletedHash () {}

	int IsEmpty () {
		return empty;
	}

	void swap (UncompletedHash &withMe) {
		char temp[sizeof(UncompletedHash)];
		memmove (temp, this, sizeof(UncompletedHash));
		memmove (this, &withMe, sizeof(UncompletedHash));
		memmove (&withMe, temp, sizeof(UncompletedHash));
	}

	void AddHash (HashHolder &addMe) {
		hashes.Insert (addMe);
	}

	void Load (Chunk &input) {
		source.Load (input);
		empty = false;
	}
};

// set of hashes that still need to be run
typedef DistributedQueue<UncompletedHash> HashesToRunList;


#endif //_CHUNKHEADER_H_
