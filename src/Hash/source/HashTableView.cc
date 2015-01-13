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

#include "HashTableView.h"

void HashTableView :: ExtractAllSegments (HashTableSegment *intoMe) {

	allSegments.MoveToStart ();
	int i = 0;
	while (allSegments.RightLength ()) {
		allSegments.Remove (intoMe[i]);
		i++;
	}
}

void HashTableView :: AddStorage (TwoWayList <HashTableSegment> &addMe) {
	addMe.swap (allSegments);
}

void HashTableView :: Replace (int whichOne, HashTableSegment &replaceWithMe) {

	allSegments.MoveToStart ();
	for (int i = 0; i < whichOne; i++) {
		allSegments.Advance ();
	}
	HashTableSegment temp;
	allSegments.Remove (temp);
	allSegments.Insert (replaceWithMe);
}

void HashTableView :: CloneOne (int whichOne, HashTableSegment &cloneIntoMe) {

	allSegments.MoveToStart ();
	for (int i = 0; i < whichOne; i++) {
		allSegments.Advance ();
	}
	HashTableSegment temp (allSegments.Current ());
	temp.swap (cloneIntoMe);
}

HashTableView :: HashTableView (HashTableView &cloneMe) {

	TwoWayList <HashTableSegment> newList;
	cloneMe.allSegments.MoveToStart ();
	while (cloneMe.allSegments.RightLength ()) {
		HashTableSegment newOne (cloneMe.allSegments.Current ());
		newList.Insert (newOne);
		newList.Advance ();
		cloneMe.allSegments.Advance ();
	}
	newList.swap (allSegments);
}

HashTableView :: HashTableView () {}

HashTableView :: ~HashTableView () {}

void HashTableView :: swap (HashTableView &withMe) {
	allSegments.swap (withMe.allSegments);
}

