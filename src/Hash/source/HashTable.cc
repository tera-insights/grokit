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

#include "HashTable.h"
#include "Errors.h"

using namespace std;
#include <iostream>

void HashTable :: EnterReader (HashTableView &myView) {

	FATALIF (!IsAllocated (), "Can't do an op on an un-initialized hash table!");

	// clone the current hash table
	pthread_mutex_lock (myMutex);
	HashTableView temp (*currentTable);
	pthread_mutex_unlock (myMutex);

	// and return the clone!
	temp.swap (myView);
}
 
int HashTable :: CheckOverFull (int *segList, double max_fill_rate) {
	pthread_mutex_lock (myMutex);
	int i = 0, count = 0;
	for (currentTable->allSegments.MoveToStart (); currentTable->allSegments.RightLength (); currentTable->allSegments.Advance (), i++) {
		if (currentTable->allSegments.Current ().CheckOverFull (max_fill_rate)) {
			segList[i] = 1;	
			count++;
		} else {
			segList[i] = 0;
		}
	}

	pthread_mutex_unlock (myMutex);
	return count;
}

int HashTable :: CheckOutOne (int *theseAreOK, HashTableSegment &checkMeOut) {

	FATALIF (!IsAllocated (), "Can't do an op on an un-initialized hash table!");

	// first, figure out all of the OK segments
	int numWanted = 0;
	int goodOnes[NUM_SEGS];
	for (int i = 0; i < NUM_SEGS; i++) {
		if (theseAreOK[i] == 1) {
			goodOnes[numWanted] = i;
			numWanted++;
		}
	}

	// now, try them one-at-a-time, in random order
	pthread_mutex_lock (myMutex);
	while (1) {

		// try each of the desired hash table segments, in random order
		for (int i = 0; i < numWanted; i++) {
		
			// randomly pick one of the guys in the list
			int rangeSize = numWanted - i;
			int whichIndex = i + (lrand48() % rangeSize);

			// move him into the current slot
			int whichToChoose = goodOnes[whichIndex];
			goodOnes[whichIndex] = goodOnes[i];
			goodOnes[i] = whichToChoose;

			// try him
			if (!writeLocked[whichToChoose]) {

				// he is open, so write lock him
				writeLocked[whichToChoose] = 1;

				// and return him
				currentTable->CloneOne (whichToChoose, checkMeOut);
				pthread_mutex_unlock (myMutex);
				return whichToChoose;
			}
		}			
			
		// if we got here, then every one that we want is write locked.  So
		// we will go to sleep until one of them is unlocked, at which point
		// we will wake up and try again...
		pthread_cond_wait (signalWriters, myMutex);	
	}
}

void HashTable :: CheckIn (int whichEntry) {

	FATALIF (!IsAllocated (), "Can't do an op on an un-initialized hash table!");

	// just note that no one is writing this one, then signal all potential writers
	pthread_mutex_lock (myMutex);
	writeLocked[whichEntry] = 0;
	pthread_cond_broadcast (signalWriters);
	pthread_mutex_unlock (myMutex);

}

void HashTable :: Replace (int whichEntry, HashTableSegment &replaceWithMe) {

	FATALIF (!IsAllocated (), "Can't do an op on an un-initialized hash table!");

	// add the new one in, then signal all potential writers
	pthread_mutex_lock (myMutex);
	writeLocked[whichEntry] = 0;
	currentTable->Replace (whichEntry, replaceWithMe);
	pthread_cond_broadcast (signalWriters);
	pthread_mutex_unlock (myMutex);
}

HashTable :: HashTable () {

	myMutex = 0;
	signalWriters = 0;
}

HashTable :: ~HashTable () {
	
	if (myMutex == 0)
		return;

	pthread_mutex_lock (myMutex);
	(*numCopies)--;
	if (numCopies > 0) {
		pthread_mutex_unlock (myMutex);
		return;
	}

	pthread_mutex_unlock (myMutex);
	pthread_mutex_destroy (myMutex);
	delete numCopies;
	delete myMutex;
	pthread_cond_destroy (signalWriters);
	delete signalWriters;
	delete currentTable;
	delete [] writeLocked;	
}

// this structure stores all of the zero'ed out segments
struct WorkToDo {

	pthread_mutex_t *myMutex;
	int numToDo;
	TwoWayList <HashTableSegment> allSegments;

	WorkToDo () {
		numToDo = NUM_SEGS;
		myMutex = new pthread_mutex_t;
		pthread_mutex_init(myMutex, NULL);
	}

	~WorkToDo () {
		pthread_mutex_destroy (myMutex);
		delete myMutex;
	}
};

void *DoSegments (void *arg) {

	WorkToDo *myWork = (WorkToDo *) arg;
	while (1) {

		// first, see if there is any work to do
		pthread_mutex_lock (myWork->myMutex);
		if (myWork->numToDo > 0) {
			myWork->numToDo--;
		} else {
			pthread_mutex_unlock (myWork->myMutex);
			return NULL;
		}

		// if we got here, there is work to do...
		pthread_mutex_unlock (myWork->myMutex);

		// create a new segment
		HashTableSegment temp;
		temp.Allocate ();
		temp.ZeroOut ();

		// and add him in
		pthread_mutex_lock (myWork->myMutex);
		myWork->allSegments.Insert (temp);
		pthread_mutex_unlock (myWork->myMutex);
	}
}

int HashTable :: IsAllocated () {

	if (myMutex == 0)
		return 0;
	else
		return 1;
}

void HashTable :: swap (HashTable &withMe) {

	char temp[sizeof (HashTable)];
	memmove (temp, &withMe, sizeof (HashTable));
	memmove (&withMe, this, sizeof (HashTable));
	memmove (this, temp, sizeof (HashTable));
}

void HashTable :: Clone (HashTable &copyMe) {

	// empty ourselves out
	HashTable temp;
	swap (temp);

	if (!copyMe.IsAllocated ())
		FATAL ("No copying an unallocated hash table!\n");

	// lock the guy we are copying
	pthread_mutex_lock (copyMe.myMutex);
	(*copyMe.numCopies)++;
	
	// do a shallow copy
	memmove (this, &copyMe, sizeof (HashTable));

	// done!
	pthread_mutex_unlock (copyMe.myMutex);
	
}

void HashTable :: Allocate (int numThreads) {

	FATALIF (IsAllocated (), "Can't allocate the hash table twice!\n");

	// set up the concurrency control stuff
	myMutex = new pthread_mutex_t;
	signalWriters = new pthread_cond_t;		
	numCopies = new int;
	currentTable = new HashTableView;
	*numCopies = 1;
	pthread_mutex_init (myMutex, NULL);
	pthread_cond_init (signalWriters, NULL);
	writeLocked = new int[NUM_SEGS];
	for (int i = 0; i < NUM_SEGS; i++) {
		writeLocked[i] = 0;
	}

	// this struct will mark the progress of the allocating/zeroing
	WorkToDo temp;

	// create a number of threads to do the work
	pthread_t threads[numThreads - 1];
	for (int i = 0; i < numThreads - 1; i++) {
		pthread_create(&(threads[i]), NULL, DoSegments, (void *) &temp); 
	}

	// do some work ourselves
	DoSegments ((void *) &temp);
	
	// now, the work is done; wait for everyone to come back
	for (int i = 0; i < numThreads - 1; i++) {
		pthread_join (threads[i], NULL);
	}

	// and set up the actual hash table!
	currentTable->AddStorage (temp.allSegments);
}
