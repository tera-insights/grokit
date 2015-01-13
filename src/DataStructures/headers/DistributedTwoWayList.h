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

#ifndef _DIST_TWO_WAY_LIST_H
#define _DIST_TWO_WAY_LIST_H

#include "DistributedCounter.h"
#include "TwoWayList.h"
#include "TwoWayList.cc"

template <class Type>
class DistributedTwoWayList {

	DistributedCounter* refCount;
	TwoWayList<Type>* t;

public:
	// type definitions to make our life easier
	typedef Type element;

	// do a deep copy of the param that is sent in
	void copy (DistributedTwoWayList &me);

	// basic constructor function
	DistributedTwoWayList ();

	// deconstructor function
	virtual ~DistributedTwoWayList ();

	// swap operator
	void swap (DistributedTwoWayList & List);

	// deep copy (needs CopyFrom on Type)
	void CopyFrom(DistributedTwoWayList & List);

	// remove all the content
	void Clear(void);

	// add to current pointer position
	void Insert (Type & Item);

	// add at the end of the list
	void Append (Type & Item);

	// look at the current item
	Type &Current ();

	// remove from current position
	void Remove (Type & Item);

	// move the current pointer position backward through the list
	void Retreat ();
	void Clean();

	// move the current pointer position forward through the list
	void Advance ();

	// operations to check the size of both sides
	int LeftLength ();
	int RightLength ();
	int Length();

	// check if empty
	bool IsEmpty();

	// operations to consult state
	bool AtStart ();
	bool AtEnd ();

	// operations to swap the left and right sides of two lists
	void SwapLefts (DistributedTwoWayList & List);
	void SwapRights (DistributedTwoWayList & List);

	// operations to move the the start of end of a list
	void MoveToStart ();
	void MoveToFinish ();

	// removes the contents of one list from another
	void Subtract (DistributedTwoWayList & takeMeOut);

	// fuction to merge two lists (steals the content from the argument
	void SuckUp(DistributedTwoWayList& suckMe);
	void Lock() {refCount->Lock();}
	void Unlock() {refCount->Unlock();}

private:
	DistributedTwoWayList(DistributedTwoWayList&);
	DistributedTwoWayList operator = (DistributedTwoWayList&);
};

#endif
