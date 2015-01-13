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

#ifndef _DISTRIBUTED_Q_C
#define _DISTRIBUTED_Q_C

#include "DistributedQueue.h"
#include "Swap.h"

#include <stdlib.h>
#include <iostream>
#include <pthread.h>

template <class Type> bool
DistributedQueue <Type> :: AtomicRemove (Type &item) {

	bool returnVal = false;
	pthread_mutex_lock (myMutex);
	myList->MoveToStart ();
	if (myList->RightLength ()) {
		myList->Remove (item);
		returnVal = true;
	}
	pthread_mutex_unlock (myMutex);
	return returnVal;
}

template <class Type> void
DistributedQueue <Type> :: Add (Type &item) {

	pthread_mutex_lock (myMutex);
	myList->MoveToFinish ();
	myList->Insert (item);
	myList->MoveToStart ();
	pthread_mutex_unlock (myMutex);

}

template <class Type> void
DistributedQueue <Type> :: Clone (DistributedQueue &me) {

	pthread_mutex_lock (myMutex);
	(*refCount)--;
	if (*refCount == 0) {
		delete refCount;
		refCount = NULL;
		pthread_mutex_unlock (myMutex);
		pthread_mutex_destroy (myMutex);
		free (myMutex);
		delete myList;
	}
	else {
		pthread_mutex_unlock (myMutex);
	}

	refCount = me.refCount;
	myList = me.myList;
	myMutex = me.myMutex;

	pthread_mutex_lock (myMutex);
	(*refCount)++;
	pthread_mutex_unlock (myMutex);
}

template <class Type> void
DistributedQueue <Type> :: swap (DistributedQueue &me) {

    SWAP_memmove(DistributedQueue, me);
}

template <class Type>
DistributedQueue <Type> :: DistributedQueue () {

	myMutex = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
	pthread_mutex_init (myMutex, NULL);
	refCount = new int;
	*refCount = 1;
	myList = new TwoWayList <Type>;
}

template <class Type>
DistributedQueue <Type> :: ~DistributedQueue () {

	pthread_mutex_lock (myMutex);
	(*refCount)--;
	if (*refCount == 0) {
		delete refCount;
		refCount = NULL;
		pthread_mutex_unlock (myMutex);
		pthread_mutex_destroy (myMutex);
		free (myMutex);
		delete myList;
	}
	else {
		pthread_mutex_unlock (myMutex);
	}
}

template <class Type> int
DistributedQueue <Type> :: Length () {

	pthread_mutex_lock (myMutex);
	myList->MoveToStart ();
	int res = myList->RightLength ();
	pthread_mutex_unlock (myMutex);
	return res;
}

#endif

