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

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "HashTableMacros.h"
#include "HashTableView.h"

// this is the central hash table class.  It is assumed that there will be one of these
// in the system, and that various copies (clones) of the central table will be passed
// around throughout the system
class HashTable {

private:

	// each entry is one iff the particular HashTableSegment is write locked
	int *writeLocked;

	// this is the current version of the hash table
	HashTableView *currentTable;	
	
	// mutex that protects the hash table
	pthread_mutex_t *myMutex;

	// condition variable used to signal those who are now blocked waiting for a
	// segment
	pthread_cond_t *signalWriters;
	
	// tells how many copies of this object are out there
	int *numCopies;

public:

	// this must be called before someone starts probing the hash table... the caller gets
	// back a HashTableView that he can then query.  The hash table view is essentially
	// a list of HashTableSegment clones.  Each clone has a counter of readers and writers,
	// that count all of the readers and writers who are looking at any one of the clones.
	// When the number of readers and writers goes down to zero, then the HashTableSegment 
	// can be killed.
	//
	// This function creates a new reader clone of each hash table segment, and returns the
	// list of clones as a HashTableView. 
	void EnterReader (HashTableView &myView);

	// this is called when you want to modify one of the segments in the hash table.  You
	// give an array theseAreOK of ints of len NUM_SEGS where the i^th integer is one iff you
	// would accept the i^th hash table segement.  What the function does is to choose one
	// of the acceptable segments, clone it, and give it back to you, putting it into
	// checkMeOut.  At this point, you lock out any other writers from writing to the hash
	// table segment, until it goes out of scope and the destructor is called.  Note that
	// this call blocks if there is not any hash table segment that the caller would accept
	// that is also available for writing.
	int CheckOutOne (int *theseAreOK, HashTableSegment &checkMeOut);

	// this is called when you want to replace a particular hash table segment with a new one.
	// When Replace is called, the hash table thows away the current HashTableSegment in the
	// specified slot.  Note that the assumption is that the specified HashTableSegment has
	// already been checked out before Replace is called; this ensures that there are no other
	// writers who will suddenly have an outdated copy after you have replaced the current one
	// Also note that after the call to Replace, the caller is assumed to have checked in the
	// segment so that they no longer own it as a writer
	void Replace (int whichSlot, HashTableSegment &replaceWithMe);

	// simply releases a write lock on the segment
	void CheckIn (int whichEntry);

	// makes a shallow copy of the hash table
	void Clone (HashTable &fromMe);
	void copy(HashTable &fromMe){ Clone(fromMe); }

	// creates an empty hash table
	HashTable ();

	// allocates the hash table having NUM_SEGS segments of specified size; uses the specified
	// number of threads to zero out and prepare the hash table... this number needs to be at
	// least 1 (a 1 means that no additional threads other than this one are spawned to do 
	// the zeroing).
	void Allocate (int numThreads);

	// tells us if the hash table has been allocated
	int IsAllocated ();

	// destructor
	~HashTable ();

	// standard swap function
	void swap (HashTable &withMe);

	// takes the input list and puts a 1 in the ith slot if the ith segment is currently over-full;
	// returns the number of overfull segments (so the number of 1s is returned)
	int CheckOverFull (int *segList, double max_fill_rate=MAX_FILL_RATE);
};

#endif
