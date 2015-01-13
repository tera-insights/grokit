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

#ifndef BI_H
#define BI_H

#include <map>

#include "Constants.h"
#include "Bitstring.h"
#include "BString.h"
#include "Column.h"
#include <cassert>
#include "Iterator.h"

#include <cstddef>
#include <utility>

class Column;

/**
    This iterator is a compression scheme for bitstring integer patterns which have repetitions. A simple
    scheme to store pattern and number of times it is repeated. BString.h has wrapper classes for keeping
    this (pattern, times) combination.

    We store object length in the header just once so that we know what size to read while reading.

    All relevant functions are inlined and defined in this header file
*/

class BStringIterator {

private:

    // Iterator
    Iterator it;

    // pattern start count
    unsigned int startCount;

    // pattern end count
    unsigned int endCount;

    // while Insert() if pattern received is same as the last one, just increment the count
    // so keep track of last seen bitstring pattern
    Bitstring mLastSeenPattern;

    // Needed for insert, because we may have random pattern in our last seen pattern buffer
    // initially which may by chance match with user given pattern. This variable also useful
    // to skip advance for the first time since we dont want advance if store is empty
    bool onceWritten;

    // num tuples in local buffer;
    // This is not total count when you have fragments
    int numTuples;

    // tuple start count, this is local to every BStringIterator, not to be copied or swapped
    // this will essetially be helping to remove usage of AtUnwrittenByte
    int tupleCount;

    // This keep information about num tuples for all fragments and objlen
    FragmentsTuples fragTuples;

    // Below variables are for checkpointing only
    unsigned int c_startCount;
    unsigned int c_endCount;
    Bitstring c_mLastSeenPattern;
    bool c_onceWritten;
    int c_numTuples;
    int c_tupleCount;

public:

  /**
    creates a bitstring iterator for the given column... the requests for data that
    are sent to the column are of size stepSize.  iterateMe is consumed.
    This constructor is needed if you want to write in column. The pattern decides the
    size of the object which is needed while Insert(). sizeofPattern is for particular use mode
    where we set single pattern sizeofPattern times (initialization). So we have two use modes,

    1) BStringIterator (iter, pattern) // pattern is not used for setting this pattern in memory, but for
       size of the object. Then subsequent calls of Insert() will add data

    2) BStringIterator (iter, pattern, noOfTimes) // here we set pattern in memory noOfTimes

    3) DO NOT use it for reading purpose
  */
    BStringIterator (Column &iterateMe, Bitstring& pattern, uint64_t sizeofPattern = 0, int stepSize = COLUMN_ITERATOR_STEP);

  /**
    This is read only constructor, size of object is read from the header of column
    Don't use it if you plan to Insert(), because it has no idea of object length to write
  */
    BStringIterator (Column &iterateMe, int _numTuples, int _objLen = 8 /*Sarvesh remove default, temp*/, int stepSize = COLUMN_ITERATOR_STEP);

    // This iterates from [fragmentStart, fragmentEnd]
    // BStringIterator (Column &iterateMe, int fragmentStart, int fragmentEnd, int _numTuples, int _objLen, int stepSize = COLUMN_ITERATOR_STEP);

    // To support user to create black object and CreateDeepCopy
    BStringIterator ():
        it(),
        startCount(),
        endCount(),
        mLastSeenPattern(),
        onceWritten(false),
        numTuples(-1),
        tupleCount(-1),
        fragTuples()
    {}

    // destructor... if there is a column left in the BStringIterator, it will be
    // destroyed as well
    ~BStringIterator ();

    // tells the iterator that we are done iterating... any new bytes written past
    // the end of the column will be included in the column from now on.  The column
    // is taken out of the iterator and put into iterateMe
    void Done (Column &iterateMe);

    // tells the iterator that we are done iterating... any new bytes written past
    // the end of the column will be included in the column from now on. This is different
    // from above in a sense that it does not swap out column. We need that now because
    // we store iterator in the Chunk instead of Column for bitmap.
    void Done ();

    // advance to the next object in the column...
    void Advance ();

    // advance from within the insert function
    void AdvanceForInsert ();

    // add a new data object into the column at the current position, overwriting the
    // bytes that are already there.  Note that if the size of addMe differs from the
    // size of the object that is already there, addMe will over-run part of the next
    // object.  Sooooo... overwrite existing objects in the column with care!
    void Insert (Bitstring &addMe);

    // returns true if the object under the cursor has never been written to and so
    // it should not be read (it is undefined what happens if you read it)
    int AtUnwrittenByte ();
    int AtEndOfColumn();

    // returns the data object at the current position in the column...
    const Bitstring& GetCurrent ();

    // Special function to switch to  BitstirngIter from the disk.
    // The current bitstirng Iter is assumed to have the correct num of tuples but not the
    // correct column
    // This functionalty is needed since the initialization from a column has to be delayed
    // util the column is loaded fromthe disk
    // ASSUMPTION: if the column is invalid, the function does nothing
    void ConvertFromCol(Column& realColumn);

    // Set the counters, start and end reading from the column chunk
    // Set the count extracting from the data stream
    // Set the last seen pattern from the data stream
    void SetCount();

    // Get number of tuples
    // This is not total count when you have fragments
    int GetNumTuples ();

/**
  create deep copy from 'fromMe'. Everything is created new down the hierarchy,
  basically all iterator states are replicated, new Column and MMappedStorage is
  created with partial data + states copied. Storage is write only for this iterator.
  The storage (data) is copied only up to the current position of fromMe. This is used
  (for example) in the join, where you find a tuple that matches with more than one tuple
  on the RHS and so now you need to start making multiple copies of values.

  Here is the short usage,
    BStringIterator<Type> myIter;
    // make sure fromMe.Done(col) is NOT called before making deep copy
    // after below line, myIter is ready to add more data using Insert(), since
    // it is pointing at the end of new replicated partial storage.
    myIter.CreateDeepCopy (fromMe); // myIter will contain write only storage
                                    // and ready to add more data using Insert()
    // Now start Inserting into myIter
*/
    void CreateDeepCopy (BStringIterator& fromMe);

    void CheckpointSave ();

    void CheckpointRestore ();

    // swap
    void swap (BStringIterator& swapMe);

    void copy (BStringIterator& copyMe);

    // Make sure you call this once you are done writing this much
    void MarkFragment (bool firstTime);
    void MarkFragmentTuples ();

    void SetFragmentRange(int start, int end);

    void SetFragmentsTuples (FragmentsTuples& _tuples);

    FragmentsTuples& GetFragmentsTuples ();

    // Used for debugging
    typedef std::map<Bitstring, int64_t> QueryInfoMap;

    QueryInfoMap DebugInfo(void);
};

// Override global swap
void swap( BStringIterator & a, BStringIterator & b );

/*** Here goes the inline definitions **/

inline
void swap( BStringIterator & a, BStringIterator & b ) {
    a.swap(b);
}

inline int BStringIterator :: AtUnwrittenByte () {
    return it.AtUnwrittenByte ();
}

inline int BStringIterator :: AtEndOfColumn () {
    return (numTuples <= tupleCount);
}


inline
int BStringIterator::GetNumTuples () {
    return numTuples;
}

inline
const Bitstring& BStringIterator::GetCurrent () {
  return mLastSeenPattern;
}

inline
void BStringIterator :: Insert (Bitstring &addMe) {

    if (it.IsInvalid ())
        return;

    // Reset first time we start writing
    if (!onceWritten) {
        numTuples = 0;
    }

    numTuples++;

    assert (it.IsWriteOnly() == true);

    int objLen = it.GetObjLen();

    // If pattern is same, update the count and return
    if (onceWritten && mLastSeenPattern == addMe) {
        switch(objLen) {
            case 4:
                if (startCount < (unsigned short)-1) {
                    startCount++;
                    BString16_16 b16((unsigned short)(addMe.GetInt64()), (unsigned short)startCount);
                    *((BString16_16 *) it.GetData()) = b16;
                    return;
                }
                break;

            case 8:
                if (startCount < (unsigned int)-1) {
                    startCount++;
                    BString32_32 b32((unsigned int)(addMe.GetInt64()), (unsigned int)startCount);
                    *((BString32_32 *) it.GetData()) = b32;
                    return;
                }
                break;

            case 12:
                if (startCount < (unsigned int)-1) {
                    startCount++;
                    BString32_64 b64(addMe.GetInt64(), startCount);
                    *((BString32_64 *) it.GetData()) = b64;
                    return;
                }
                break;

            default :
                assert(0);
        }
    }

    // reset the pattern counter
    startCount = 0;

    // Advance our position
    if (onceWritten) {
        it.Advance();
        // SetCount(); Not needed I think
    }

    it.EnsureWriteSpace();

    // and write the object
    switch(objLen) {
        case 4:
            {
                BString16_16 b16((unsigned short)(addMe.GetInt64()), 0);
                *((BString16_16 *) it.GetData()) = b16;
                break;
            }

        case 8:
            {
                BString32_32 b32((unsigned int)(addMe.GetInt64()), 0);
                *((BString32_32 *) it.GetData()) = b32;
                break;
            }

        case 12:
            {
                BString32_64 b64(addMe.GetInt64(), 0);
                *((BString32_64 *) it.GetData()) = b64;
                break;
            }
    }

    //remember the pattern
    mLastSeenPattern = addMe;

    onceWritten = true;
}

inline
void BStringIterator :: Advance () {

    if (it.IsInvalid ())
        return;

    tupleCount++;
    // advance our position
    if (!it.IsWriteOnly()) {
        if (startCount < endCount) {
            startCount++;
            return;
        }
    } else {
        // never advance for Insert
        return;
    }

    if( tupleCount < numTuples ) {
        it.Advance();
        SetCount();
    }
}

// Set the count extracting from the data stream
// Set the last seen pattern from the data stream
inline
void BStringIterator :: SetCount () {

    char* myData = it.GetData();
    int objLen = it.GetObjLen();

    startCount = 0;
    if (objLen == 4) {
        endCount = ((BString16_16*) myData)->GetCount();
        mLastSeenPattern = *((BString16_16 *) myData);
    }
    else if (objLen == 8) {
        endCount = ((BString32_32*) myData)->GetCount();
        mLastSeenPattern = *((BString32_32 *) myData);
    }
    else if (objLen == 12) {
        endCount = ((BString32_64*) myData)->GetCount();
        mLastSeenPattern = *((BString32_64 *) myData);
    }
    else
        assert(0);
}

inline
void BStringIterator :: MarkFragment (bool firstTime) {

    if (it.IsInvalid ())
        return;

    if (firstTime == false)
        fragTuples.SetTuplesCount(numTuples);

    // Advance our position
    if (firstTime == false) {
        it.Advance();
    }

    // Make sure this mark is only after advance
    it.MarkFragment();
    // Set this to false because in new memory location we have not written yet because we can not
    // rely on last seen pattern check in Insert, because same pattern can be written in new location
    onceWritten = false;
    startCount = 0;
}

inline
void BStringIterator :: MarkFragmentTuples () {

    if (it.IsInvalid ())
        return;

    fragTuples.SetTuplesCount(numTuples);
}

inline
void BStringIterator :: SetFragmentsTuples (FragmentsTuples& _tuples) {
    fragTuples = _tuples;
}

inline
FragmentsTuples& BStringIterator :: GetFragmentsTuples () {
    return fragTuples;
}

#endif
