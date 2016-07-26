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

#include "BStringIterator.h"
#include "Bitstring.h"
#include "Errors.h"
#include "Swap.h"

#include <iostream>
#include <cassert>
#include <cinttypes>
#include <map>

using namespace std;

// 2nd argument is 0 for inner iterator because this is fixed type size
BStringIterator :: BStringIterator (Column &iterateMe, Bitstring& pattern, uint64_t sizeofPattern, uint64_t stepSize) :
    it(iterateMe, 0, stepSize),
    startCount(0),
    endCount(0),
    mLastSeenPattern(0),
    denseQueries(0),
    onceWritten(false),
    numTuples(0),
    tupleCount(0),
    fragTuples()
{
        // The Bitstring is dense for all queries at creation time.
        denseQueries.SetAll();

        if (it.IsInvalid ())
            return;

        assert(stepSize > 256); // Just because of few assumptions below while writing header and advancing

        // reset the pattern counter
        int objLen = 0;

        // If pattern count received is zero, then find the object length, otherwise
        // assume the fixed object length in else block
        // Also compute the endCount for future use while Insert
        if (sizeofPattern == 0) {
            if (!(pattern.GetInt64() >> 16)) {
                objLen = 4;
                endCount = (((unsigned int)1)<<16) - 1;
            }
            else if (!(pattern.GetInt64() >> 32)) {
                objLen = 8;
                endCount = (unsigned int)-1;
            }
            else {
                objLen = 12;
                endCount = (unsigned int)-1;
            }
        } else { // assume fixed maximum possible object length
            // because we know we only have one object in memory, so no space problem
            objLen = 12;
            endCount = (unsigned int)-1;
        }

        it.SetObjLen (objLen);
        fragTuples.SetHeader1(objLen);

        // Ensure the first object space is entirely in memory
        it.EnsureFirstObjectSpace (objLen);

        // write the pattern (if received from user) in the memory
        if (sizeofPattern != 0) {
            BString32_64 b(pattern.GetInt64(), sizeofPattern-1);
            *((BString32_64 *) it.GetData()) = b;
            // Update the numTuples
            numTuples = (int)sizeofPattern;
            onceWritten = true;
        }

        if( numTuples > 0 ) {
            // remember the last seen pattern for write mode (this constructor)
            if (objLen == 4)
                mLastSeenPattern = *((BString16_16 *) it.GetData());
            else if (objLen == 8)
                mLastSeenPattern = *((BString32_32 *) it.GetData());
            else if (objLen == 12)
                mLastSeenPattern = *((BString32_64 *) it.GetData());
            else
                assert(0);

            if (sizeofPattern == 0) {
                // start and endcount must be overwritten if read only mode
                SetCount();
            }
        }

        tupleCount = 0; // will be used to see if this iterator reached at the end tuple
    }


BStringIterator :: BStringIterator (Column &iterateMe, uint64_t _numTuples, uint64_t _objLen, uint64_t stepSize) :
    it(iterateMe, 0, stepSize),
    startCount(0),
    endCount(0),
    mLastSeenPattern(0),
    onceWritten(false),
    numTuples(_numTuples),
    tupleCount(0),
    fragTuples()
{

    if (it.IsInvalid ())
        return;

    if (it.GetFirstInvalidByte() == 0){
        WARNING("For some reason the bistring is empty and it should not be")
            return;
    }

    assert(stepSize > 256);

    // reset the pattern counter
    it.SetObjLen (_objLen);

    // Ensure the first object space is entirely in memory (even when we know objLen will
    // not exceed stepSize) just for consistency
    it.EnsureFirstObjectSpace ();

    // start and endcount must be overwritten if read only mode
    SetCount();
    tupleCount = 0; // will be used to see if this iterator reached at the end tuple
}

void
BStringIterator :: SetFragmentRange (uint64_t start, uint64_t end) {

    assert(!it.IsInvalid());
    //assert(!(startCount > 0));
    startCount = 0;

    it.SetFragmentRange(start, end);

    if (it.GetFirstInvalidByte() == 0){
        WARNING("For some reason the bistring is empty and it should not be")
            return;
    }

    numTuples = fragTuples.GetTupleCount(start, end);
    assert(numTuples >= 0);

    // Ensure the first object space is entirely in memory (even when we know objLen will
    // not exceed stepSize) just for consistency
    it.EnsureFirstObjectSpace ();

    // start and endcount must be overwritten if read only mode
    SetCount();
    tupleCount = 0; // will be used to see if this iterator reached at the end tuple
}

/*
   BStringIterator :: BStringIterator (Column &iterateMe, uint64_t fragmentStart, uint64_t fragmentEnd, uint64_t _numTuples, uint64_t _objLen, uint64_t stepSize) :
   it (iterateMe, fragmentStart, fragmentEnd, 0, stepSize) {

   if (it.IsInvalid ())
   return;

   if (it.GetFirstInvalidByte() == 0){
   WARNING("For some reason the bistring is empty and it should not be")
   return;
   }

   assert(stepSize > 256);

// reset the pattern counter
startCount = 0;

onceWritten = false;

//numTuples = _fragTuples.GetTupleCount(fragmentStart, fragmentEnd);
//objLen = _fragTuples.GetHeader1();
numTuples = _numTuples;
it.SetObjLen (_objLen);

// Ensure the first object space is entirely in memory (even when we know objLen will
// not exceed stepSize) just for consistency
it.EnsureFirstObjectSpace ();

// start and endcount must be overwritten if read only mode
SetCount();
}

*/

BStringIterator :: ~BStringIterator () {
}

void BStringIterator :: Done (Column &iterateMe) {

    //fragTuples.Save ();
    it.Done (iterateMe);
}

void BStringIterator :: Done () {

    //fragTuples.Save ();
    it.Done ();
    SetCount();
    tupleCount = 0;
    onceWritten = false;
}

void BStringIterator :: CreateDeepCopy (BStringIterator& fromMe) {

    // do shallow copy first
    //memmove (this, &fromMe, sizeof (BStringIterator));

    it.CreateDeepCopy(fromMe.it);

    startCount = 0;

    onceWritten = false;
    numTuples = fromMe.numTuples;

    int objLen = it.GetObjLen();
    if (objLen == 4) {
        endCount = (((unsigned int)1)<<16) - 1;
    }
    else if (objLen == 8) {
        endCount = (unsigned int)((((uint64_t)1)<<32) - 1);
    }
    else if (objLen == 12) {
        endCount = (unsigned int)((((uint64_t)1)<<32) - 1);
    } else {
        assert(0);
    }
    tupleCount = 0;
    fragTuples = fromMe.fragTuples;
}

void BStringIterator :: swap (BStringIterator& swapMe) {

    it.swap (swapMe.it);
    SWAP_STD(startCount, swapMe.startCount);
    SWAP_STD(endCount, swapMe.endCount);
    mLastSeenPattern.swap(swapMe.mLastSeenPattern);
    SWAP_STD(onceWritten, swapMe.onceWritten);
    SWAP_STD(numTuples, swapMe.numTuples);
    tupleCount = 0; // start iterating from the start
    swapMe.tupleCount=0;
    fragTuples.swap(swapMe.fragTuples);
}

void BStringIterator :: copy (BStringIterator& copyMe) {

    it.copy (copyMe.it);
    startCount = copyMe.startCount;
    endCount = copyMe.endCount;
    mLastSeenPattern = copyMe.mLastSeenPattern;
    onceWritten = copyMe.onceWritten;
    numTuples = copyMe.numTuples;
    fragTuples = copyMe.fragTuples;
    //assert(copyMe.numTuples != 0);
    tupleCount = 0; // start iterating from the start
    copyMe.tupleCount=0;
}

void BStringIterator :: CheckpointSave () {
    c_startCount = startCount;
    c_endCount = endCount;
    c_mLastSeenPattern.copy(mLastSeenPattern);
    c_onceWritten = onceWritten;
    c_numTuples = numTuples;
    c_tupleCount = tupleCount;
    // We don't need to checkpoint fragTuple as this is same all the time
    it.CheckpointSave();
}

void BStringIterator :: CheckpointRestore () {
    startCount = c_startCount;
    endCount = c_endCount;
    mLastSeenPattern.copy(c_mLastSeenPattern);
    onceWritten = c_onceWritten;
    numTuples = c_numTuples;
    tupleCount = c_tupleCount;
    it.CheckpointRestore();
}

void BStringIterator :: ConvertFromCol(Column& realColumn) {

    if (it.IsInvalid ())
        return;
    assert(tupleCount != -1);

    tupleCount = 0; // start iterating from the start
    it.ConvertFromCol (realColumn);
    // We dont need to remember last seen pattern as this will be used only in read mode
    SetCount();
}

auto BStringIterator :: DebugInfo(void) -> QueryInfoMap {
    QueryInfoMap ret;
    Bitstring curQuery;
    int64_t count;

    // Do a checkpoint save first.
    CheckpointSave();
    Done();

    char * data;
    int objLen = it.GetObjLen();

    // Go through the iterator and log each pattern set
    while( ! it.AtUnwrittenByte() ) {
        data = it.GetData();

        switch( objLen ) {
            case 4:
                count = ((BString16_16 *) data)->GetCount();
                curQuery = *((BString16_16 *) data);
                break;
            case 8:
                count = ((BString32_32 *) data)->GetCount();
                curQuery = *((BString32_32 *) data);
                break;
            case 12:
                count = ((BString32_64 *) data)->GetCount();
                curQuery = *((BString32_64 *) data);
                break;
            default:
                FATAL("Bitstring Iterator has underlying storage of unexpected size %d", objLen);
        }

        if( curQuery.IsEmpty() ) {
            // Add to number of empty tuples
            ret[curQuery] += count;
        }
        else {
            // Keep track per query
            Bitstring tmp;
            while( !curQuery.IsEmpty() ) {
                tmp = curQuery.GetFirst();
                ret[tmp] += count;
            }
        }

        it.Advance();
    }

    // Restore the checkpoint before exiting
    CheckpointRestore();

    return ret;
}
