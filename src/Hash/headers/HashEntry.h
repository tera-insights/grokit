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

// the systen't central hash table is a 2-D array of HashEntry objects...

#ifndef HASH_ENTRY_H
#define HASH_ENTRY_H

#include "Overflow.h"
#include "HashTableMacros.h"

#include <iostream>

class HashEntry {

    // this is the smallest offet value that must be written externally outside of the hash
    static constexpr unsigned int MAX = 1023;

    friend class HashTableSegment;
    unsigned int info;
    VAL_TYPE value;

    public:

    // the following functions put info into the hash entry...

    // mark this one as invalid
    inline void SetEmpty ();

    // tell this hash entry it has data from a LHS/RHS join record (this automattically
    // marks it as a "start" entry)
    inline void SetRHS ();
    inline void SetLHS ();

    // this tells the entry it continues an existing record
    inline void SetContinuation ();

    // put the way point ID into the hash entry (only do this for a "start" entry!)
    inline void SetWayPointID (unsigned int wayPointID);

    // tell this hash entry how far it is from the correct position in the hash table
    // again, this is only for "start" entries.  slot is the slot where this entry appears
    // in the hash table, and putExtraHere is used for overflow info.  What happens is that
    // if the distance from the correct position can be written in 10 bits, it is.  If it can't,
    // then it is written to the pbject putExtraHere
    inline void SetDistFromCorrectPos (unsigned int distance, HT_INDEX_TYPE slot, Overflow &putExtraHere);

    // tell this hash entry how for to the next entry in this record... used for both
    // start and continuation records.  Note that just like the above function, we deal with
    // distances greater than 10 bits by writing them into the putExtraHere object
    inline void SetDistToNextEntry (unsigned int distance, HT_INDEX_TYPE slot, Overflow &putExtraHere);
    inline void SetDistToNextEntryToZero ();

    // tell what attribute we are putting into the record.  Only for continuation reocords
    // (by definition, start records always hold the bitmap)
    inline void SetAttributeID (unsigned int whichID);

    // these functions just extract all of the above info out
    inline bool IsUsed ();
    inline bool IsStartOfTuple ();
    inline unsigned int GetWayPointID ();
    inline bool IsRHS ();
    inline bool IsLHS ();
    inline unsigned int GetAttributeID ();

    // since these two may need to go to the Overflow object to retreive distances greater than
    // 10 bits, they also accept the overflow object as input
    inline unsigned int GetDistFromCorrectPos (HT_INDEX_TYPE slot, Overflow &putExtraHere);
    inline unsigned int GetDisttoNextEntry (HT_INDEX_TYPE slot, Overflow &putExtraHere);

    // this extracts the actual data from a hash entry, writing sizeof (VAL_TYPE) bytes to
    // the location that is pointed to
    inline void Extract (void *writeHere);

    // this puts actual data into the hash entry, reading sizeof (VAL_TYPE) bytes from the
    // pointed to location
    inline void PutIn (void *fromHere);

    // zero it out... makes it unused, with nothing in there
    inline void EmptyOut ();

} __attribute__((__packed__));


// Most of these operations involve "bit twiddling" on the first 32 bits of the hash entry.
// The last four operations may involve going to the Overflow object in order to obtain or
// store a distance that is greater than 1023.

// In the first 32 bits of the hash entry (that is, the "info" art of the entry),
// the least significant two bits encode: 0 not used; 1 continuation; 2 start, LHS; 3 start, RHS
// the next 10 bits are the number of hash slots until the continuation
// in a start record, the next 10 bits are the distance from the correct position
// in a continuation record, the next 10 bits are the attribute number
// in a start record, the most significant 10 bits is the waypoint the record belongs to
inline void HashEntry :: EmptyOut () {info = 0;}
inline void HashEntry :: SetEmpty () {info &= (~0 ^ 3);}
inline void HashEntry :: SetRHS () {info |= 3;}
inline void HashEntry :: SetLHS () {SetEmpty (); info |= 2;}
inline void HashEntry :: SetContinuation () {SetEmpty (); info |= 1;}
inline void HashEntry :: SetAttributeID (unsigned int whichID) {info = (info & 4290777087) | ((whichID & 1023) << 12);}
inline void HashEntry :: SetWayPointID (unsigned int distance) {info = (info & 4194303) | ((distance & 1023) << 22);}
inline bool HashEntry :: IsUsed () {return (info & 3) >= 1;}
inline bool HashEntry :: IsStartOfTuple () {return (info & 3) >= 2;}
inline bool HashEntry :: IsRHS () {return (info & 3) == 3;}
inline bool HashEntry :: IsLHS () {return (info & 3) == 2;}
inline unsigned int HashEntry :: GetAttributeID () {return (info & 4190208) >> 12;}
inline unsigned int HashEntry :: GetWayPointID () {return info >> 22;}
inline void HashEntry :: Extract (void *writeHere) {*((VAL_TYPE *) writeHere) = value;}
inline void HashEntry :: PutIn (void *fromHere) {value = *((VAL_TYPE *) fromHere);}
inline void HashEntry :: SetDistToNextEntryToZero () {info = (info & 4294963203) | ((0 & 1023) << 2);}

// these last four operations all may need to go to the overflow object, if the 10 bits allocated to recording
// distances to and from different positions in the array are not enough
inline void HashEntry :: SetDistToNextEntry (unsigned int distance, HT_INDEX_TYPE slot, Overflow &putExtraHere) {
    if (distance >= MAX) {
        putExtraHere.RecordDistanceToNext (slot, distance);
        distance = MAX;
    }
    info = (info & 4294963203) | ((distance & 1023) << 2);
}

inline void HashEntry :: SetDistFromCorrectPos (unsigned int distance, HT_INDEX_TYPE slot, Overflow &putExtraHere) {
    if (distance >= MAX) {
        putExtraHere.RecordDistanceFromCorrect (slot, distance);
        distance = MAX;
    }
    info = (info & 4290777087) | ((distance & 1023) << 12);
}

inline unsigned int HashEntry :: GetDisttoNextEntry (HT_INDEX_TYPE slot, Overflow &putExtraHere) {
    unsigned int distance = (info & 4092) >> 2;
    if (distance == MAX)
        return putExtraHere.GetDistanceToNext (slot);
    else
        return distance;
}

inline unsigned int HashEntry :: GetDistFromCorrectPos (HT_INDEX_TYPE slot, Overflow &putExtraHere) {
    unsigned int distance = (info & 4190208) >> 12;
    if (distance == MAX)
        return putExtraHere.GetDistanceFromCorrectPos (slot);
    else
        return distance;
}


#endif
