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

#include "HashTableSegment.h"
#include <string.h>
#include <pthread.h>
#include "MmapAllocator.h"
#include "Errors.h"
#include "Logging.h"

using namespace std;
#include <iostream>

double HashTableSegment :: globalFillRate = 0.0;

void HashTableSegment :: swap (HashTableSegment &withMe) {
	char space[sizeof (HashTableSegment)];
	memmove (space, this, sizeof (HashTableSegment));
	memmove (this, &withMe, sizeof (HashTableSegment));
	memmove (&withMe, space, sizeof (HashTableSegment));
}

void HashTableSegment :: SetOverFull () {
	*fillRate = 1.0;
}

void HashTableSegment :: CheckHasBeenCleanedBefore (int &LHS, int &RHS) {
	if (LHS < *privateLHS || RHS < *privateRHS) 
		FATAL ("How can the number of tupes removed decrease????");

	int LHSin = LHS;
	int RHSin = RHS;
	LHS = LHS - *privateLHS;
	RHS = RHS - *privateRHS;
	*privateLHS = LHSin;
	*privateRHS = RHSin;
}

int HashTableSegment :: CheckOverFull (double max_fill_rate) {
  return (*fillRate>=max_fill_rate);
}

HashTableSegment :: ~HashTableSegment () {

	// in the case that this guy was never allocated, do nothing
	if (myMutex == 0)
		return;

	// this will tell us whether this dude needs to be deallocated
	int done = 0;

	// decrement the number of references to the segment
	pthread_mutex_lock (myMutex);
	(*numReadersWriters)--;
	if (*numReadersWriters == 0)
		done = 1;
	pthread_mutex_unlock (myMutex);

	// see if we were the very last reference
	if (done) {
		pthread_mutex_destroy (myMutex);
		if (numBytes < 2097152) { 
			free (myData);
		} else {
		  //SYS_MMAP_FREE(myData, numBytes);
		  mmap_free(myData);
		}
		delete myMutex;
		myMutex = 0;
		delete numReadersWriters;
		delete fillRate;
		delete [] randomProbeSlots;
		numReadersWriters = 0;
		delete bigOffsets;
		delete privateLHS;
		delete privateRHS;
	}	
}

// this version of insert does not do the sampleing, and it returns the last slot in the segment that it wrote to
int HashTableSegment :: Insert (SerializedSegmentArray &data, HashSegmentSample &sampledCollisions) {

	// in the future, we might want to and the case where the bitstring spans multiple hash entries
	FATALIF (sizeof (Bitstring) > sizeof (VAL_TYPE), "Oops! Sampling to check for overfull assumes the bitstring fits in one hash entry!\n");

	// this is the number of collisions in the first NUM_TEST_PROBES hash attempts
	int numCollisions = 0;

	// first thing we do is probe to see if there are too many over-full entries
	for (int probeNum = 0; probeNum < NUM_TEST_PROBES; probeNum++) {

		// first thing is to get the slot we need to go to
		HT_INDEX_TYPE whichSlot = randomProbeSlots[probeNum];
		
		if (myData[whichSlot].IsUsed ()) {
			numCollisions++;
			
			// now look for the start of the tuple
			HT_INDEX_TYPE i;
			for (i = whichSlot; !myData[i].IsStartOfTuple (); i--);
	
			// extract the bitstring and the waypoint from the start of the tuple
			Bitstring tempBitstring;
			myData[i].Extract (&tempBitstring);
			HashEntrySummary mySummary (myData[i].GetWayPointID (), tempBitstring);

			// and remember them
			sampledCollisions.Insert (mySummary);
		}
	}

	for (int posInHashes = 0, posInArray = 0; posInHashes < data.lastUsedHash; posInHashes++) {

		// first thing is to compute the slot we need to go to
		HT_INDEX_TYPE whichSlot = data.allHashes[posInHashes];

		// tell the loop that are at the start of a tuple
		int startOfTuple = 1;

		// now go there and add all of the data!  Keep going until we see the start of the 2nd tuple
		for (; posInArray < data.lastUsedSeg && (startOfTuple || !data.myData[posInArray].IsStartOfTuple ()); posInArray++) {

			// mark that we are no longer at the start of the current tuple
			startOfTuple = 0;

			// try to find some space to add this thing... this involved hopping along the hash chains
			unsigned int counter = 0;
			while (whichSlot < ABSOLUTE_HARD_CAP && myData[whichSlot].IsUsed ()) {
				int dist = myData[whichSlot].GetDisttoNextEntry (whichSlot, *bigOffsets);
				if (dist == 0)
					dist = 1;
				whichSlot += dist;
				counter += dist;
			}

			// see if we went too far; we devote 10 bits to pointers, so we can go 1028 slots to find space in the hash table
			if (whichSlot >= ABSOLUTE_HARD_CAP) {
			  printf("whichSlot=%d\ncounter=%d\n", whichSlot,counter);
				cerr << "(" << *fillRate << ") (" << numCollisions << ", " << NUM_TEST_PROBES * MAX_FILL_RATE << ")\n";
				FATAL ("I ran off the end of the hash table segment when I tried to add data.\nYou are probably trying to insert too much data with the same hash key");
			}

			// got an empty space, so add the new data
			myData[whichSlot] = data.myData[posInArray];

			// if this is the first entry, then set up the back pointer
			if (myData[whichSlot].IsStartOfTuple ())
				myData[whichSlot].SetDistFromCorrectPos (counter, whichSlot, *bigOffsets);
			
			// if it is not, then set up the earlier one's forward pointer
			else {	
				myData[whichSlot - counter].SetDistToNextEntry (counter, whichSlot - counter, *bigOffsets);
			}
		}

	}

	// finally, let the caller know if we get too many collisions
	*fillRate = 1.0*numCollisions/NUM_TEST_PROBES;

	globalFillRate=globalFillRate*.9+.1*(*fillRate); // 

	return CheckOverFull();
}

HashTableSegment :: HashTableSegment (HashTableSegment &cloneMe) {

	// copy everything over
	myData = cloneMe.myData;
	myMutex = cloneMe.myMutex;
	numBytes = cloneMe.numBytes;
	randomProbeSlots = cloneMe.randomProbeSlots;
	numReadersWriters = cloneMe.numReadersWriters;
	fillRate = cloneMe.fillRate;
	bigOffsets = cloneMe.bigOffsets;
	privateLHS = cloneMe.privateLHS;
	privateRHS = cloneMe.privateRHS;

	// and increase the reference count
	pthread_mutex_lock (myMutex);
	(*numReadersWriters)++;
	pthread_mutex_unlock (myMutex);
}

HashTableSegment :: HashTableSegment () {
	myData = 0;
	myMutex = 0;
	numReadersWriters = 0;
	numBytes = 0;
	randomProbeSlots = 0;
	fillRate = 0;
	bigOffsets = 0;
	privateLHS = 0;
	privateRHS = 0;
}

void HashTableSegment :: ZeroOut (HT_INDEX_TYPE low, HT_INDEX_TYPE high) {
	for (HT_INDEX_TYPE i = low; i < ABSOLUTE_HARD_CAP && i < high; i++) {
		myData[i].EmptyOut ();	
	}
}

void HashTableSegment :: ZeroOut () {
#ifdef SLOW_HASH_INIT
	for (HT_INDEX_TYPE i = 0; i < ABSOLUTE_HARD_CAP; i++) {
		myData[i].EmptyOut ();	
	}
#else
	memset(myData, 0, ABSOLUTE_HARD_CAP*sizeof(HashEntry));
#endif
}

void HashTableSegment :: Allocate () {

	// first set up a new segment
	HashTableSegment temp;
	temp.myMutex = new pthread_mutex_t;
	pthread_mutex_init(temp.myMutex, NULL);
	temp.numReadersWriters = new int;
	*(temp.numReadersWriters) = 1;
	temp.fillRate = new double;
	*(temp.fillRate) = 0.0;
	temp.bigOffsets = new Overflow;
	temp.privateLHS = new int;
	temp.privateRHS = new int;
	*(temp.privateLHS) = 0;
	*(temp.privateRHS) = 0;

	// this sets up the list of probes
	temp.randomProbeSlots = new HT_INDEX_TYPE[NUM_TEST_PROBES];
	for (int i = 0; i < NUM_TEST_PROBES; i++) {
		temp.randomProbeSlots[i] = lrand48() % NUM_SLOTS_IN_SEGMENT;
	}

	// and swap him into us
	swap (temp);

	// now we see how much storage we need
	numBytes = ABSOLUTE_HARD_CAP * sizeof(HashEntry);
	
	// now, actually allocate the data
	// if this table is small, we will not use big pages
	if (numBytes < 2097152) { 
		myData = (HashEntry *) malloc (numBytes);
	} else {
		//myData = (HashEntry *) SYS_MMAP_ALLOC (numBytes);
		myData = (HashEntry *) mmap_alloc (numBytes, 1);
		//FATALIF( !SYS_MMAP_CHECK((void*)myData), "Could not allocate %ld MB for segments of the large hash", numBytes >> 20);
	}
}
