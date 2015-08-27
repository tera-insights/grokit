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

#ifndef HASH_MACROS
#define HASH_MACROS

// MACROS USED FOR HASHING!!

#include "Constants.h"

#include <cstdint>

// this will decide how many fragments we need to partition a bucket into
#define NUM_FRAGMENT_BITS 5

// this is a bit mask, that, when applied to an int, obtain s a number from 0 to NUM_SEGS
#define NUM_SEGS_MASK (NUM_SEGS - 1)


// this is the data type used to store actual data in the hash table
#define VAL_TYPE uint64_t

// this is the data type used to index the hash table... the hash functions should take this as an arg
#define HT_INDEX_TYPE uint64_t


// this is the number of bits needed to index all of the entried in a hash table segment
// moved to Constnts.h #define NUM_SLOTS_IN_SEGMENT_BITS 20

// this is the number of entries in a hash table segment
#define NUM_SLOTS_IN_SEGMENT (1ULL << (NUM_SLOTS_IN_SEGMENT_BITS))

// this is a bitmask that, when applied to an int, obtains a number from 0 to NUM_SLOTS_IN_SEGMENT
#define HASH_SLOTS_MASK (NUM_SLOTS_IN_SEGMENT - 1ULL)

// this takes a hash value and maps it to a slot in the segment
// Old version #define WHICH_SLOT(hash) ((hash & (HASH_SLOTS_MASK << NUM_SEGS_BITS)) >> NUM_SEGS_BITS)
#define WHICH_SLOT(hash) (hash & (HASH_SLOTS_MASK ))

// this take a hash value and figures out which segment it is in
#define WHICH_SEGMENT(hash) ((hash >> NUM_SLOTS_IN_SEGMENT_BITS) % NUM_SEGS)

// this is the absolute limit on the number of entries in a hash table segment... includes
// some scratch space at the end
#define ABSOLUTE_HARD_CAP ((HT_INDEX_TYPE)(NUM_SLOTS_IN_SEGMENT * 1.08))

// this is the maximum fill rate we allow in a segment when we add data to it
#define MAX_FILL_RATE .7

// a lower bounday to clean to to avoid oscilations and maximize CPU usilization
#define CLEAN_FILL_RATE .35

// this is the goal in terms of how full a segment should be after cleaning
#define FRAC_TO_TAKE_IT_DOWN (0.9 * CLEAN_FILL_RATE / MAX_FILL_RATE)

// this is the number of test hash table probes to make when we determine of the segment is over-full
#define NUM_TEST_PROBES 200

// this is a special number that identifies the bitmap attribute.  In the hash table, 10 bits are devoted
// to identifying which attribute is being stored.  1111111111 is reserved to indicate the bitmap
#define BITMAP 1023

#endif

