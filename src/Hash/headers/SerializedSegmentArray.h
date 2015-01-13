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

#ifndef SER_SEG_ARRAY_H
#define SER_SEG_ARRAY_H

#include <stdlib.h>
#include <string.h>
#include "HashEntry.h"

#include <iostream>

// This class holds a big long list of serialized tuples.  Tuples are first inserted
// into this list by the join operation, before they are added into the central hash table.
class SerializedSegmentArray {

    HashEntry *myData;
    int arrayLenSegs;
    int lastUsedSeg;

    // this stores all of the hash values that are present in the array
    HT_INDEX_TYPE *allHashes;
    int arrayLenHashes;
    int lastUsedHash;

    void DoubleHashArray ();
    void DoubleEntryArray ();

    friend class HashTableSegment;

    public:

    // constructor/destructor
    SerializedSegmentArray ();
    ~SerializedSegmentArray ();

    // this function is called when you want to start inserting a new tuple into the list
    inline void StartNew (HT_INDEX_TYPE hashValue, unsigned int wayPointID, int RHS,
            void *bitmapBytesToWrite, int howManyBytes);

    // this function is called when you want to add more data to a record you've started
    inline void Append (unsigned int columnID, void *bytesToWrite, int howManyBytes);

    // forgets the data in the array
    inline void EmptyOut ();

};

inline void SerializedSegmentArray :: EmptyOut () {
    lastUsedSeg = lastUsedHash = 0;
}

inline void SerializedSegmentArray :: Append (unsigned int columnID, void *bytesToWrite, int howManyBytes) {

    // loop through and add all of the bytes in
    for (int bytesWritten = 0; bytesWritten < howManyBytes; bytesWritten += sizeof (VAL_TYPE), lastUsedSeg++) {

        // if we need more space, then obtain it
        if (lastUsedSeg == arrayLenSegs)
            DoubleEntryArray ();

        // add the actual data
        myData[lastUsedSeg].PutIn ((char*)bytesToWrite + bytesWritten);

        // put in the meta-data
        myData[lastUsedSeg].SetDistToNextEntryToZero ();
        myData[lastUsedSeg].SetContinuation ();
        myData[lastUsedSeg].SetAttributeID (columnID);
    }
}

inline void SerializedSegmentArray :: StartNew (HT_INDEX_TYPE hashValue, unsigned int wayPointID, int RHS,
        void *bitmapBytesToWrite, int howManyBytes) {

    // remember the hash value
    if (lastUsedHash == arrayLenHashes)
        DoubleHashArray ();
    allHashes[lastUsedHash] = hashValue;
    lastUsedHash++;

    // loop through and add all of the bytes in
    for (int bytesWritten = 0; bytesWritten < howManyBytes; bytesWritten += sizeof (VAL_TYPE), lastUsedSeg++) {

        // if we need more space, then obtain it
        if (lastUsedSeg == arrayLenSegs)
            DoubleEntryArray ();

        // add the actual data
        myData[lastUsedSeg].PutIn ((char*)bitmapBytesToWrite + bytesWritten);

        // put in the meta-data
        myData[lastUsedSeg].SetDistToNextEntryToZero ();
        if (bytesWritten == 0) {
            if (RHS)
                myData[lastUsedSeg].SetRHS ();
            else
                myData[lastUsedSeg].SetLHS ();
            myData[lastUsedSeg].SetWayPointID (wayPointID);
        } else {
            myData[lastUsedSeg].SetContinuation ();
            myData[lastUsedSeg].SetAttributeID (BITMAP);
        }

    }

}

#endif
