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

#ifndef OVERFLOW_H
#define OVERFLOW_H

#include "HashTableMacros.h"
#include "EfficientIntToIntMap.h"

// this class is a container for hash table offsets that take up too many bits to
// be stored within the hash table itself.  In this case, they are stored externally
// in an object of type Overflow
class Overflow {

	EfficientIntToIntMap distancesToNext;
	EfficientIntToIntMap distancesFromCorrect;

public:

	// this is used to record an offset distance from one hash entry to the next
	// that is larger than 10 bits
	inline void RecordDistanceToNext (HT_INDEX_TYPE slot, unsigned int distance);

	// this is used to record the distance from the start of a record in the hash 
	// table backwards to the slot it was actually supposed to be hashed to, when
	// that distance is larger than 10 bits, so it can't be recorded in the hash itself
	inline void RecordDistanceFromCorrect (HT_INDEX_TYPE slot, unsigned int distance);

	// access a stored forward offset
	inline unsigned int GetDistanceToNext (HT_INDEX_TYPE slot);

	// access a stored backward offset
	inline unsigned int GetDistanceFromCorrectPos (HT_INDEX_TYPE slot);
	
};

inline void Overflow :: RecordDistanceToNext (HT_INDEX_TYPE slot, unsigned int distance) {
        distancesToNext.Insert (slot, distance);
}       

inline void Overflow :: RecordDistanceFromCorrect (HT_INDEX_TYPE slot, unsigned int distance) {
        distancesFromCorrect.Insert (slot, distance);
}       

inline unsigned int Overflow :: GetDistanceToNext (HT_INDEX_TYPE slot) {
        return distancesToNext.Find (slot); 
}       

inline unsigned int Overflow :: GetDistanceFromCorrectPos (HT_INDEX_TYPE slot) {
        return distancesFromCorrect.Find (slot); 
}       

#endif
