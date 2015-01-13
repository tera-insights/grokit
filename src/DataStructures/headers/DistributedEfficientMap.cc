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

#ifndef _DIST_EFFICIENT_MAP_C
#define _DIST_EFFICIENT_MAP_C

#include "DistributedEfficientMap.h"
#include "EfficientMap.cc"

// basic constructor function
template <class Key, class Data>
DistributedEfficientMap <Key, Data> :: DistributedEfficientMap () {
	refCount = new DistributedCounter(1);
	effmap = new EfficientMap<Key, Data>;
}

// basic deconstructor function
template <class Key, class Data>
DistributedEfficientMap <Key, Data> :: ~DistributedEfficientMap () {
	Clean ();
}

template <class Key, class Data> void
DistributedEfficientMap <Key, Data> :: Clean () {

	if (refCount->Decrement(1) == 0){
		delete refCount, refCount = NULL;
		if (effmap != NULL) {
			delete effmap;
			effmap = NULL;
		}
	}

}

// swap operator
template <class Key, class Data> void
DistributedEfficientMap <Key, Data> :: swap (DistributedEfficientMap & withMe) {
	char storage[sizeof (DistributedEfficientMap)];
	memmove (storage, this, sizeof (DistributedEfficientMap));
	memmove (this, &withMe, sizeof (DistributedEfficientMap));
	memmove (&withMe, storage, sizeof (DistributedEfficientMap));
}

// make the first node the current node
template <class Key, class Data> void
DistributedEfficientMap <Key, Data> :: MoveToStart ()
{
	effmap->MoveToStart();
}

template <class Key, class Data> bool
DistributedEfficientMap <Key, Data> :: AtStart ()
{
	return effmap->AtStart();
}

// make the first node the current node
template <class Key, class Data> void
DistributedEfficientMap <Key, Data> :: MoveToFinish ()
{
	effmap->MoveToFinish();
}

template <class Key, class Data> void
DistributedEfficientMap <Key, Data> :: Advance ()
{
	effmap->Advance();
}

// move backwards through the list
template <class Key, class Data> void
DistributedEfficientMap <Key, Data> :: Retreat ()
{
	effmap->Retreat();
}

template <class Key, class Data> bool
DistributedEfficientMap <Key, Data> :: AtEnd () {
	return effmap->AtEnd();
}

template <class Key, class Data> void
DistributedEfficientMap <Key, Data> :: Clear(void){
	//the implementation is more intricate
	//will be done in the future
    effmap->Clear();
}

template <class Key, class Data> int
DistributedEfficientMap <Key, Data> :: Remove (Key &key, Key &removedKey, Data &removedData) {

	effmap->Remove(key, removedKey, removedData);
}

template <class Key, class Data> Data &
DistributedEfficientMap <Key, Data> :: Find (Key &key) {

	return effmap->Find(key);
}

template <class Key, class Data> int
DistributedEfficientMap <Key, Data> :: IsThere (Key &key) {

	return effmap->IsThere(key);
}

template <class Key, class Data> void
DistributedEfficientMap <Key, Data> :: Insert (Key &key, Data &data)
{
	effmap->Insert(key, data);
}

template <class Key, class Data> void
DistributedEfficientMap <Key, Data> :: SuckUp(DistributedEfficientMap <Key, Data>& other){

	effmap->SuckUp(other.effmap);
}


template <class Key, class Data> void
DistributedEfficientMap <Key, Data> :: Print(){
	effmap->Print();
}

template <class Key, class Data> void
DistributedEfficientMap <Key, Data> :: Lock(){
	refCount->Lock();
}

template <class Key, class Data> void
DistributedEfficientMap <Key, Data> :: Unlock(){
	refCount->Unlock();
}


template <class Key, class Data> void
DistributedEfficientMap <Key, Data> :: copy(DistributedEfficientMap <Key, Data>& other){
	Clean();
	other.refCount->Increment(1);// this guarantees the other guy does not die and it does not change effmap
	refCount = other.refCount;
	effmap = other.effmap;
}



template <class Key, class Data> Data &
DistributedEfficientMap <Key, Data> :: CurrentData ()
{
	return effmap->CurrentData();
}

template <class Key, class Data> Key &
DistributedEfficientMap <Key, Data> :: CurrentKey ()
{
	return effmap->CurrentKey();
}

#endif

