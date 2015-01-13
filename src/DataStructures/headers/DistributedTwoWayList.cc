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

#ifndef _DIST_TWO_WAY_LIST_C
#define _DIST_TWO_WAY_LIST_C

#include "DistributedTwoWayList.h"
#include "TwoWayList.cc"

// basic constructor function
template <class Type>
DistributedTwoWayList <Type> :: DistributedTwoWayList ()
{
	refCount = new DistributedCounter(1);
	t = new TwoWayList<Type>;
}

template <class Type> 
void DistributedTwoWayList<Type> :: SuckUp (DistributedTwoWayList &suckMe) {
	t->SuckUp(suckMe.t);
}


template <class Type>
void DistributedTwoWayList <Type> :: Subtract (DistributedTwoWayList &me)
{
	t->Subtract(me.t);
}

template <class Type>
void DistributedTwoWayList <Type> :: copy (DistributedTwoWayList &me)
{
	t->copy(me.t);
}

// basic deconstructor function
template <class Type>
DistributedTwoWayList <Type> :: ~DistributedTwoWayList ()
{
	Clean();
}

template <class Type> void
DistributedTwoWayList <Type> :: Clean () {

        if (refCount->Decrement(1) == 0){
                delete refCount, refCount = NULL;
                if (t != NULL) {
                        delete t;
                        t = NULL;
                }
        }

}

// swap operator
template <class Type> void
DistributedTwoWayList <Type> :: swap (DistributedTwoWayList & withMe)
{
        char storage[sizeof (DistributedTwoWayList)];
        memmove (storage, this, sizeof (DistributedTwoWayList));
        memmove (this, &withMe, sizeof (DistributedTwoWayList));
        memmove (&withMe, storage, sizeof (DistributedTwoWayList));
}

template <class Type> void
DistributedTwoWayList <Type> :: copy (DistributedTwoWayList & other)
{
	Clean();
	t->copy(other.t);
	refCount->Increment(1);
}

template <class Type> void
DistributedTwoWayList <Type> :: Clear(void){
	t->Clear();
}



// make the first node the current node
template <class Type> void
DistributedTwoWayList <Type> :: MoveToStart ()
{
	t->MoveToStart();
}

// make the first node the current node
template <class Type> void
DistributedTwoWayList <Type> :: MoveToFinish ()
{
	t->MoveToFinish();
}

// determine the number of items to the left of the current node
template <class Type> int
DistributedTwoWayList <Type> :: LeftLength ()
{
	return t->LeftLength();
}

// determine the number of items to the right of the current node
template <class Type> int
DistributedTwoWayList <Type> :: RightLength ()
{
	return t->RightLength();
}


template <class Type> int
DistributedTwoWayList <Type> :: Length ()
{
	return t->Length();
}

template <class Type> bool
DistributedTwoWayList <Type> :: IsEmpty()
{
	return t->IsEmpty();
}


template <class Type> bool
DistributedTwoWayList <Type> :: AtStart ()
{
	return t->AtStart();
}


template <class Type> bool
DistributedTwoWayList <Type> :: AtEnd ()
{
	return t->AtEnd();
}

// swap the right sides of two lists
template <class Type> void
DistributedTwoWayList <Type> :: SwapRights (DistributedTwoWayList & List)
{
	t->SwapRights(List.t);
}

// swap the leftt sides of the two lists
template <class Type> void
DistributedTwoWayList <Type> :: SwapLefts (DistributedTwoWayList & List)
{
	t->SwapLefts(List.t);
}

// move forwards through the list
template <class Type> void
DistributedTwoWayList <Type> :: Advance ()
{
	t->Advance();
}

// move backwards through the list
template <class Type> void
DistributedTwoWayList <Type> :: Retreat ()
{
	t->Retreat();
}

// insert an item at the current poition
template <class Type> void
DistributedTwoWayList <Type> :: Insert (Type & Item)
{
	t->Insert(Item);
}

template <class Type> void
DistributedTwoWayList <Type> :: Append (Type & Item)
{
	t->Append();
}

template <class Type> Type &
DistributedTwoWayList <Type> :: Current ()
{
	return t->Current();
}

// remove an item from the current poition
template <class Type> void
DistributedTwoWayList <Type> :: Remove (Type & Item)
{
	t->Remove(Item);
}

#endif
