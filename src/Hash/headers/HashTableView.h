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

#ifndef HASH_VIEW
#define HASH_VIEW

#include "HashEntry.h"
#include "HashTableSegment.h"
#include "TwoWayList.cc"

// This is a snapshot of the state of a hash table at a particular instant of time.
// It is used both by the HashTable object to store all of its data, and to give
// away a copy of the table for a reader to look at
class HashTableView {

private:

	TwoWayList <HashTableSegment> allSegments;

protected:

	friend class HashTable;

	// this replaces the i^th segment in the list
	void Replace (int whichOne, HashTableSegment &replaceWithMe);
	
	// this clones the i^th segment in the list
	void CloneOne (int whichOne, HashTableSegment &cloneIntoMe);

	// this creates a hash table view that is a clone of the one passed in
	HashTableView (HashTableView &cloneMe);

	// adds the set of segments into the view as its storage
	void AddStorage (TwoWayList <HashTableSegment> &storage);

public:

	// this accepts an array of pointers to lists of HashTableEntrys, and
	// fills up that array so that it contains the current hash table
	void ExtractAllSegments (HashTableSegment *intoMe);

	// creates an empty hash table view
	HashTableView ();

	// destructor
	~HashTableView ();

	// swap two hash table views
	void swap (HashTableView &withMe);

};

#endif

