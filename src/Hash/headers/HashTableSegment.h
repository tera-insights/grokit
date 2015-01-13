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

#ifndef HASH_SEGMENT
#define HASH_SEGMENT

#include "HashTableMacros.h"
#include "HashEntry.h"
#include "SerializedSegmentArray.h"
#include "HashData.h"

// this class encapsulates one of the segments of the main hash table.  It is broken
// into segments so that it is easy for writers to lock individual segments, and to
// swap new versions in wholesale
class HashTableSegment {

 public:
  static double globalFillRate; // the global fill rate of the global hash

protected:

	// this mutex protects updates to the number of readers and writers
	pthread_mutex_t *myMutex;

	// tells us the number of concurrent readers/writers accessing this segment
	int *numReadersWriters;

	// this is the actual data in the hash table segment
	HashEntry *myData;

	// this is the number of bytes in the hash segment
	HT_INDEX_TYPE numBytes;

	// this tells us which slots to probe in the hash table when testing for fullness
	HT_INDEX_TYPE *randomProbeSlots;

	// how full is this segment
	double* fillRate;

	// tells us if it's been cleaned; for debugging
	int *privateLHS;
	int *privateRHS;

	// this is used to store offsets from one hash entry to another that are more than
	// 10 bits, and hence cannot be stored within the actual hash table itself
	Overflow *bigOffsets;

public:

	// mark this segment as being overfull; used when someone tries to add data and finds there is too much there
	void SetOverFull ();

	// check if it is overfull
	// the optional argument can change the threshold at which the segment is delclared full
	int CheckOverFull (double max_fill_rate=MAX_FILL_RATE);

	// for debugging
	void CheckHasBeenCleanedBefore (int &LHS, int &RHS);

	// this is called to probe the hash table segment.  Starting at curSlot, we begin looking for an attribute value
	// from a tuple that hashed to position "goal".  The att we are interested in is whichAtt, and the value must be
	// from the join waypoint hainvg join waypoint ID wayPointID.  The result is written to serializeHere; the number
	// bytes serialized are returned (a 0 indicates that we could not find the desired data in the table).  If wayPointID
	// is -1, then ANY waypoint is accepted; the waypoint found is put into wayPointID.  Whether or not this is a LHS tuple
	// is returned inside of LHS.  Done is set to 1 if the last attribute in the tuple has been found; it is zero otherwise.
	inline int Extract (void *serializeHere, HT_INDEX_TYPE &curSlot, HT_INDEX_TYPE goal, int &wayPointID, int whichAtt, int &LHS, int &done);

	// insert the list of serialized tuples into the hash table... in the first, we assume that all inserts are sequential
	// in terms of hash ID, and we zero out as we go
	inline void Insert (SerializedSegmentArray &data);

	// in the second, we track the first NUM_TEST_PROBES insert attempts... if more than NUM_TEST_PROBES * MAX_FILL_RATE of them
	// result in a collision, then a 1 is returned... in any case, a list of the collisions found is returned in mySample
	int Insert (SerializedSegmentArray &data, HashSegmentSample &mySample);

	// swap two segments
	void swap (HashTableSegment &withMe);

	// this is needed to get things to compile (because for various reasons we have to put these into a DataC container)
	// BUT the routine should never be called... if it is, we abort
	void copy (HashTableSegment &withMe) {
		FATAL ("copy for HashTableSegment was supposed to be a dummy op!\n");
	}

	// create a HashTableSegment by cloning the input
	HashTableSegment (HashTableSegment &cloneMe);

	// create an empty HashTableSegment
	HashTableSegment ();

	// allocates this guy
	void Allocate ();

	// clears him out
	void ZeroOut ();

	// clears him out, but only in a specific range (inclusive)
	void ZeroOut (HT_INDEX_TYPE low, HT_INDEX_TYPE high);

	// destructor
	~HashTableSegment ();

};


// returns number of bytes extracted on success, 0 otherwise
inline int HashTableSegment :: Extract (void *serializeHere, HT_INDEX_TYPE &curSlot,
	HT_INDEX_TYPE goal, int &wayPointID, int whichAtt, int &isLHS, int &done) {

	// if numBytes = 0 it means that we have not found anything from our tuple yet
	int numBytes = 0;

	// loop until we find the first one with the correct hash value
	while (1) {

		// if this one is empty, just return that we are done
		if (!myData[curSlot].IsUsed ()) {
			return numBytes;
		}

		// if we are currently serializing and the current attribute does not match, we are done
		if ((numBytes > 0 || whichAtt != BITMAP) && whichAtt != myData[curSlot].GetAttributeID ()) {
			done = 0;
			return numBytes;
		}

		// if we are currently serializing and the attribute matches, then add it in
		if ((numBytes > 0 || whichAtt != BITMAP) && whichAtt == myData[curSlot].GetAttributeID ()) {
			myData[curSlot].Extract ((char*)serializeHere + numBytes);
			numBytes += sizeof (VAL_TYPE);
		}

		// if this is the start of a tuple and it has the correct hash and we are looking for a bitmap, add it in
		if (myData[curSlot].IsStartOfTuple () && curSlot - myData[curSlot].GetDistFromCorrectPos (curSlot, *bigOffsets) == goal
			&& whichAtt == BITMAP && (myData[curSlot].GetWayPointID () == wayPointID || wayPointID == -1)) {

			wayPointID = myData[curSlot].GetWayPointID ();
			isLHS = myData[curSlot].IsLHS ();
			myData[curSlot].Extract ((char*)serializeHere + numBytes);
			numBytes += sizeof (VAL_TYPE);
		}

		// now we advance... if we are at the end of a tuple and we have been serializing, then advance one
		if (myData[curSlot].GetDisttoNextEntry (curSlot, *bigOffsets) == 0) {

			curSlot += 1;
			done = 1;

			// if we have been serializing, we are now done
			if (numBytes > 0) {
				return numBytes;
			}

		} else {
			curSlot += myData[curSlot].GetDisttoNextEntry (curSlot, *bigOffsets);
		}
	}
}

// this version of insert does not do the sampleing, and it returns the last slot in the segment that it wrote to
inline void HashTableSegment :: Insert (SerializedSegmentArray &data) {

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

			// see if we went too far
			if (whichSlot >= ABSOLUTE_HARD_CAP) {
				FATAL ("I ran off the end of the hash table segment when I tried to add data.\n");
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
}

// STL-compatible swap function
inline
void swap( HashTableSegment& a, HashTableSegment& b ) {
    a.swap(b);
}

#endif

