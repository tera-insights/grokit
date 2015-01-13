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

#ifndef _DIST_Q_H
#define _DIST_Q_H

#include <iostream>
#include <pthread.h>

#include "Swap.h"
#include "TwoWayList.h"

// This is a templte for a thread-safe distributed queue template,
// built upon the two-way-list.  You can replicate it many times thru
// the "Clone" operation, and it maintains an internal reference count
// to safely clean up memory when the last reference is done with
template <class Type>
class DistributedQueue {

public:
	// return the number of items in the queue
	int Length ();

	// pull an item off of the head of the queue; return "true" if something was there,
	// false if nothing was pulled off
	bool AtomicRemove (Type &item);

	// add an item to the tail of the queue
	void Add (Type &item);

	// make a clone or a copy of the queue (usually so you can give the
	// clone to another thread)... these do the same thing
	void Clone (DistributedQueue &cloneMe);
	void copy (DistributedQueue &cloneMe) {Clone (cloneMe);}

	// and of course, the swap operation
	void swap (DistributedQueue &withMe);

	// constructor and destructor
	DistributedQueue ();
	virtual ~DistributedQueue ();

private:
	int *refCount;
	TwoWayList <Type> *myList;
	pthread_mutex_t *myMutex;
};

#endif
