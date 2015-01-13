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

#ifndef SEGMENT_META_DATA_H
#define SEGMENT_META_DATA_H

#include "HashTableSegment.h"

// this is the data that we need to remember for each segment
struct SegmentMetaData {

	// these tell us the state of each of the wounded waypoints in this segment 
	// (see HashTableCleanerManager.h for a decription of all of these)
	JoinWayPointIDList woundedBeingCleaned;
	JoinWayPointIDList woundedAcked;
	JoinWayPointIDList woundedDropped;

	// tell us the state of each of the dead waypoints
	JoinWayPointIDList deadBeingCleaned;
	JoinWayPointIDList deadAcked;
	JoinWayPointIDList deadDropped;

	// tells us the state of all dead query exits
	QueryExitContainer exitsBeingCleaned;
	QueryExitContainer exitsCleaned;

	// this is the hash table segment that should actually be put into the hash table 
	HashTableSegment oneToPutBack;

	SegmentMetaData () {}

	~SegmentMetaData () {}

	void swap (SegmentMetaData &withMe) {
		char temp[sizeof (SegmentMetaData)];
		memmove (temp, &withMe, sizeof (SegmentMetaData));
		memmove (&withMe, this, sizeof (SegmentMetaData));
		memmove (this, temp, sizeof (SegmentMetaData));
	}

	inline void PrintList (FILE *foo, JoinWayPointIDList &printMe) {
		printMe.MoveToStart ();
		//for (printMe.MoveToStart (); printMe.RightLength (); printMe.Advance ()) 
			; //fprintf (foo, "%d ", printMe.Current ());
		int temp = printMe.RightLength ();
		fprintf (foo, "%d\n", temp);

	}

	void Print (FILE *foo) {
		fprintf (foo, "woundedBeingCleaned: ");
		PrintList (foo, woundedBeingCleaned);
		fprintf (foo, "woundedAcked: ");
		PrintList (foo, woundedAcked);
		fprintf (foo, "woundedDropped: ");
		PrintList (foo, woundedDropped);
		fprintf (foo, "deadBeingCleaned: ");
		PrintList (foo, deadBeingCleaned);
		fprintf (foo, "deadAcked: ");
		PrintList (foo, deadAcked);
		fprintf (foo, "deadDropped: ");
		PrintList (foo, deadDropped);
		fflush (foo);
	}
};

#endif
