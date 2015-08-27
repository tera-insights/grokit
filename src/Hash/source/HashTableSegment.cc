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
#include "Random.h"

#include <iostream>
#include <algorithm>

std::atomic<double> HashTableSegment :: globalFillRate(0.0);

void HashTableSegment :: updateGlobalFillRate(double rate) {
	double old = globalFillRate.load();
	double updated = ((1.0 - FILL_RATE_UPDATE_FACTOR) * old) +
					 (FILL_RATE_UPDATE_FACTOR * rate);

	while(!globalFillRate.compare_exchange_weak(old, updated)) {
		updated = ((1.0 - FILL_RATE_UPDATE_FACTOR) * old) +
				  (FILL_RATE_UPDATE_FACTOR * rate);
	}
}

void HashTableSegment :: swap (HashTableSegment &withMe) {
    data.swap(withMe.data);
}

void HashTableSegment :: SetOverFull () {
	data->fillRate = 1.0;
}

void HashTableSegment :: CheckHasBeenCleanedBefore (int &LHS, int &RHS) {
	if (LHS < data->privateLHS || RHS < data->privateRHS)
		FATAL ("How can the number of tupes removed decrease????");

	int LHSin = LHS;
	int RHSin = RHS;
	LHS = LHS - data->privateLHS;
	RHS = RHS - data->privateRHS;
	data->privateLHS = LHSin;
	data->privateRHS = RHSin;
}

int HashTableSegment :: CheckOverFull (double max_fill_rate) {
  return (data->fillRate >= max_fill_rate);
}

// this version of insert does not do the sampleing, and it returns the last slot in the segment that it wrote to
int HashTableSegment :: Insert (SerializedSegmentArray &segments, HashSegmentSample &sampledCollisions) {

	// in the future, we might want to and the case where the bitstring spans multiple hash entries
	FATALIF (sizeof (Bitstring) > sizeof (VAL_TYPE), "Oops! Sampling to check for overfull assumes the bitstring fits in one hash entry!\n");

	// this is the number of collisions in the first NUM_TEST_PROBES hash attempts
	int numCollisions = 0;

	// first thing we do is probe to see if there are too many over-full entries
	for (int probeNum = 0; probeNum < NUM_TEST_PROBES; probeNum++) {

		// first thing is to get the slot we need to go to
		HT_INDEX_TYPE whichSlot = data->randomProbeSlots[probeNum];
		FATALIF(whichSlot >= ABSOLUTE_HARD_CAP,
			"Slot to probe for probe number %d is past end of data slots", probeNum);

		if (data->myData[whichSlot].IsUsed ()) {
			numCollisions++;

			// now look for the start of the tuple
			HT_INDEX_TYPE i;
			for (i = whichSlot; !data->myData[i].IsStartOfTuple (); i--);

			// extract the bitstring and the waypoint from the start of the tuple
			Bitstring tempBitstring;
			data->myData[i].Extract (&tempBitstring);
			HashEntrySummary mySummary (data->myData[i].GetWayPointID (), tempBitstring);

			// and remember them
			sampledCollisions.Insert (mySummary);
		}
	}

	for (int posInHashes = 0, posInArray = 0; posInHashes < segments.lastUsedHash; posInHashes++) {

		// first thing is to compute the slot we need to go to
		HT_INDEX_TYPE whichSlot = segments.allHashes[posInHashes];

		// tell the loop that are at the start of a tuple
		int startOfTuple = 1;

		// now go there and add all of the data!  Keep going until we see the start of the 2nd tuple
		for (; posInArray < segments.lastUsedSeg && (startOfTuple || !segments.myData[posInArray].IsStartOfTuple ()); posInArray++) {

			// mark that we are no longer at the start of the current tuple
			startOfTuple = 0;

			// try to find some space to add this thing... this involved hopping along the hash chains
			unsigned int counter = 0;
			while (whichSlot < ABSOLUTE_HARD_CAP && data->myData[whichSlot].IsUsed ()) {
				int dist = data->myData[whichSlot].GetDisttoNextEntry (whichSlot, data->bigOffsets);
				if (dist == 0)
					dist = 1;
				whichSlot += dist;
				counter += dist;
			}

			// see if we went too far; we devote 10 bits to pointers, so we can go 1028 slots to find space in the hash table
			if (whichSlot >= ABSOLUTE_HARD_CAP) {
                printf("whichSlot=%d\ncounter=%d\n", whichSlot,counter);
                std::cerr << "(" << data->fillRate << ") (" << numCollisions << ", " << NUM_TEST_PROBES * MAX_FILL_RATE << ")\n";
                FATAL ("I ran off the end of the hash table segment when I tried to add data.\nYou are probably trying to insert too much data with the same hash key");
			}

			// got an empty space, so add the new data
			data->myData[whichSlot] = segments.myData[posInArray];

			// if this is the first entry, then set up the back pointer
			if (data->myData[whichSlot].IsStartOfTuple ())
				data->myData[whichSlot].SetDistFromCorrectPos (counter, whichSlot, data->bigOffsets);

			// if it is not, then set up the earlier one's forward pointer
			else {
				data->myData[whichSlot - counter].SetDistToNextEntry (counter, whichSlot - counter, data->bigOffsets);
			}
		}

	}

	// finally, let the caller know if we get too many collisions
	data->fillRate = 1.0*numCollisions/NUM_TEST_PROBES;

	updateGlobalFillRate(data->fillRate);

	return CheckOverFull();
}

HashTableSegment :: HashTableSegment (const HashTableSegment &cloneMe)
{
	data = cloneMe.data;
}

HashTableSegment :: HashTableSegment ():
	data(nullptr)
{ }

void HashTableSegment :: ZeroOut (HT_INDEX_TYPE low, HT_INDEX_TYPE high) {
	for (HT_INDEX_TYPE i = low; i < ABSOLUTE_HARD_CAP && i < high; i++) {
		data->myData[i].EmptyOut ();
	}
}

void HashTableSegment :: ZeroOut () {
    FATALIF(!data, "Attempting to zero out unallocated HashTableSegment");
#ifdef SLOW_HASH_INIT
	for (HT_INDEX_TYPE i = 0; i < ABSOLUTE_HARD_CAP; i++) {
		data->myData[i].EmptyOut ();
	}
#else
	memset(data->myData.get(), 0, data->numBytes);
#endif
}

void HashTableSegment :: Allocate () {

	SharedData *nData = new SharedData;

	// now we see how much storage we need
	nData->numBytes = ABSOLUTE_HARD_CAP * sizeof(HashEntry);

	// now, actually allocate the data
	// if this table is small, we will not use big pages
	if (nData->numBytes < 2097152 /* 2MB */) {
		nData->myData = data_ptr_t(new HashEntry[ABSOLUTE_HARD_CAP], std::default_delete<HashEntry[]>());
	} else {
		//myData = (HashEntry *) SYS_MMAP_ALLOC (numBytes);
		nData->myData = data_ptr_t((HashEntry *) mmap_alloc(nData->numBytes, 1),
			[](HashEntry* ptr) { mmap_free(ptr); });
		//FATALIF( !SYS_MMAP_CHECK((void*)myData), "Could not allocate %ld MB for segments of the large hash", numBytes >> 20);
	}

	nData->randomProbeSlots.reset(new HT_INDEX_TYPE[NUM_TEST_PROBES]);
	for (size_t i = 0; i < NUM_TEST_PROBES; i++) {
		nData->randomProbeSlots[i] = RandInt(0, NUM_SLOTS_IN_SEGMENT - 1);
	}

	data.reset(nData);
}
