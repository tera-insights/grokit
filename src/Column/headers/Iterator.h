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
#ifndef I_H
#define I_H

#include "Constants.h"
#include "Column.h"

/**
  This is common inner implementation for all iterators. All relevant things are inlined
  and kept in header file
  */

class Column;

class Iterator {

#include "IteratorPrivate.h"

    public:

        // creates a column iterator for the given column... the requests for data that
        // are sent to the column are of size stepSize.  iterateMe is consumed.
        Iterator (Column &iterateMe, int minByteToGetLength, int stepSize = COLUMN_ITERATOR_STEP);

        // We only iterate through [fragmentStart, fragmentEnd]
        Iterator (Column &iterateMe, int fragmentStart, int fragmentEnd, int minByteToGetLength, int stepSize = COLUMN_ITERATOR_STEP);

        Iterator ();
        // destructor... if there is a column left in the ColumnIterator, it will be
        // destroyed as well
        ~Iterator ();

        // tells the iterator that we are done iterating... any new bytes written past
        // the end of the column will be included in the column from now on.  The column
        // is taken out of the iterator and put into iterateMe
        void Done (Column &iterateMe);
        void Done ();

        // advance to the next object in the column...
        void Advance ();

        // start from the beggining
        // usefull to read what we wrote
        void Restart();

        // returns true if the object under the cursor has never been written to and so
        // it should not be read (it is undefined what happens if you read it)
        int AtUnwrittenByte ();

        // create deep copy
        void CreateDeepCopy (Iterator& fromMe);

        // create shallow copy
        void CreateShallowCopy (Iterator& copyMe);

        // create checkpoint
        void CheckpointSave ();

        // restore previous saved checkpoint
        void CheckpointRestore ();

        // swap
        void swap (Iterator& swapMe);

        // copy
        void copy (Iterator& copyMe);

        // Explicitly set the object length
        void SetObjLen (int len);

        // Get the object length
        int GetObjLen();

        // Get the data stream handle
        char* GetData ();

        // Advance by given amount
        void AdvanceBy (int len);

        // If I am write only iterator.
        bool IsWriteOnly ();

        // get the first invalid byte in the stream
        int GetFirstInvalidByte ();

        /** Below Ensure functions are to consolidate all logic at once place with easy interface
          for other iterators
         **/

        // This makes sure we have objLen or asked number of bytes available with us
        void EnsureFirstObjectSpace (int len = -1);

        // This makes sure we have write space for the object we are going to write
        void EnsureWriteSpace ();

        // Ensure len amount of space and increment by increment
        void EnsureSpace (int len, int increment);

        // Ensure header space and increment by stepSize
        void EnsureHeaderSpace ();

        // Do we have invalid column OR column at all?
        bool IsInvalid ();

        // Mark fragment
        void MarkFragment ();

        // Set fragment range
        void SetFragmentRange (int start, int end);

        // Get the number of fragments
        int GetNumFragments();

        //lace with this column
        void ConvertFromCol(Column& realColumn);
};

// Override global swap
void swap( Iterator & a, Iterator & b );

/** Here goes the inline definitions */

inline
void swap( Iterator & a, Iterator & b ) {
    a.swap(b);
}

inline
void Iterator :: SetObjLen (int len) {
    //FATALIF(len < 0, "\n cuspos = %d, firstInvalidByte=%d, last objlen=%d, crappy length=%d", curPosInColumn, firstInvalidByte, objLen, len);
    objLen = len;
}

inline
int Iterator :: GetFirstInvalidByte () {
    return firstInvalidByte;
}

inline
int Iterator :: GetObjLen () {
    return objLen;
}

inline
bool Iterator :: IsWriteOnly () {
    return isWriteOnly;
}

inline
char* Iterator :: GetData () {
    return myData;
}

inline
int Iterator :: AtUnwrittenByte () {
    return (curPosInColumn >= colLength || colLength == 0 );
}

inline
void Iterator :: Advance () {

    if (isInValid)
        return;

    AdvanceBy (objLen);

    // if we advance past the end of the column, don't get any data
    if (curPosInColumn >= colLength) {
        return;
    }

    // make sure we have not advanced too far
    EnsureSpace (myMinByteToGetLength, bytesToRequest);

    // make sure that we have fully loaded the next object into memory
    EnsureSpace (objLen, objLen);
}

inline
void Iterator :: AdvanceBy (int len) {

    // advance our position
    curPosInColumn += len;
    myData += len;
}

inline
void Iterator :: EnsureSpace (int len, int increment) {

    //printf("\nEnsurespace, curPosInColumn = %d, len = %d, increment=%d, firstInvalidByte=%d, curPosInColumn + len+%d", curPosInColumn, len, increment, firstInvalidByte, curPosInColumn + len); fflush(stdout);
    if (curPosInColumn + len > firstInvalidByte) {
        int requestLen = increment;
        if (requestLen < objLen)
            requestLen = objLen;
        myData = myColumn.GetNewData (curPosInColumn, requestLen);
        //firstInvalidByte += requestLen;
        firstInvalidByte = curPosInColumn + requestLen;
    }
}

inline
void Iterator :: EnsureHeaderSpace () {

    EnsureSpace (myMinByteToGetLength, bytesToRequest);
}

inline
void Iterator :: EnsureWriteSpace () {

    EnsureSpace (objLen, bytesToRequest);
}

inline
bool Iterator :: IsInvalid () {
    FATALIF(!isInValid && !myColumn.IsValid(), "Column should be valid");

    return isInValid;
}

inline
void Iterator :: MarkFragment () {

    myColumn.GetFragments().MarkFragment (curPosInColumn);
}

inline
int Iterator :: GetNumFragments() {

    return myColumn.GetFragments().GetNumFragments();
}
// no need to save and restore all the state (cannot change)

inline
void Iterator :: CheckpointSave () {
    c_myData = myData;
    //	c_bytesToRequest = bytesToRequest;
    c_curPosInColumn = curPosInColumn;
    //	c_firstInvalidByte = firstInvalidByte;
    c_objLen = objLen;
    //	c_colLength = colLength;
    //	c_isWriteOnly = isWriteOnly;
    //	c_myMinByteToGetLength = myMinByteToGetLength;
    //	c_isInValid = isInValid;
}

inline
void Iterator :: CheckpointRestore () {
    myData = c_myData;
    //	bytesToRequest = c_bytesToRequest;
    objLen = c_objLen;
    curPosInColumn = c_curPosInColumn;
    firstInvalidByte = curPosInColumn;
    //	firstInvalidByte = c_firstInvalidByte;
    //	colLength = c_colLength;
    //	isWriteOnly = c_isWriteOnly;
    //	myMinByteToGetLength = c_myMinByteToGetLength;
    //	isInValid = c_isInValid;
    //
    //int requestLen = bytesToRequest;
    //myData = myColumn.GetNewData (curPosInColumn, requestLen);
    //firstInvalidByte = curPosInColumn + requestLen;
    //Advance();
}


#endif

