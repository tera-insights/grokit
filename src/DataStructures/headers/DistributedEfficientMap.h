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

#ifndef _DIST_EFFICIENT_MAP_H
#define _DIST_EFFICIENT_MAP_H

#include "DistributedCounter.h"
#include "EfficientMap.h"

/** Class to provide a thread safe interface over the EfficientMap.
    
    Only cration, copy and detruction are actually thread safe. The
    rest of the methods need calls to Lock() and Unlock() to be thread
    safe.

*/

template <class Key, class Data>
class DistributedEfficientMap {

	DistributedCounter* refCount;

	EfficientMap<Key, Data>* effmap;

	// aux function to get rid of data if last copy
	void Clean();
	
public:

	// constructor and destructor
	DistributedEfficientMap ();
	~DistributedEfficientMap ();

	// Locking/unlocking primitives. Use for all actions except cration and destruction anc copy/swap
	void Lock();
	void Unlock();

	// remove all the content
	void Clear(void);
       
	// inserts the key/data pair into the structure
	void Insert (Key &key, Data &data);

	// eat up another map
	// plays nicely and removes duplicates
	void SuckUp(DistributedEfficientMap& other);

	// get the content from another map (without destroying it)
	void copy(DistributedEfficientMap& other);

	// removes one (any) instance of the given key from the map...
	// returns a 1 on success and a zero if the given key was not found
	int Remove (Key &findMe, Key &putKeyHere, Data &putDataHere);

	// attempts to locate the given key
	// returns 1 if it is, 0 otherwise
	int IsThere (Key &findMe);

	// returns a reference to the data associated with the given search key
	// if the key is not there, then a garbage (newly initialized) Data item is
	// returned.  "Plays nicely" with IsThere in the sense that if IsThere found
	// an item, Find will immediately return that item w/o having to locate it
	Data &Find (Key &findMe);

	// swap two of the maps
	void swap (DistributedEfficientMap &withMe);

	///////////// ITERATOR INTERFAACE //////////////
	// look at the current item
	Key& CurrentKey ();
	Data& CurrentData ();

	// move the current pointer position backward through the list
	void Retreat ();

	// move the current pointer position forward through the list
	void Advance ();
	
	// operations to consult state
	bool AtStart ();
	bool AtEnd ();

	// operations to move the the start of end of a list
	void MoveToStart ();
	void MoveToFinish ();

	// debugging function
	void Print();

};

#endif
