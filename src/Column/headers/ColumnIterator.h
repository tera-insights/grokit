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

#ifndef CI_H
#define CI_H

#include <assert.h>
#include <cstddef>
#include "Constants.h"
#include "Iterator.h"

// Default functions for Serialize/Deserialize
template <class DataType>
size_t Serialize(char * buffer, const DataType& src) {
    DataType* asTypePtr = reinterpret_cast<DataType*>(buffer);
    *asTypePtr = src;
    return sizeof(DataType);
}

template <class DataType>
size_t Deserialize(const char * buffer, DataType& dest) {
    const DataType* asTypePtr = reinterpret_cast<const DataType*>(buffer);
    dest = *asTypePtr;
    return sizeof(DataType);
}

/**
  This is iterator for flat datatypes (which do not have pointer inside as data members)
  All relevant functions are made inline
  */
class Column;

template <class DataType, int headerSize = 0, int dtSize = sizeof(DataType) >
class ColumnIterator {

    protected:
        Iterator it;

        DataType curItem;

    public:

        // creates a column iterator for the given column... the requests for data that
        // are sent to the column are of size stepSize.  iterateMe is consumed.
        ColumnIterator (Column &iterateMe, int stepSize = COLUMN_ITERATOR_STEP);

        // This iterates from [fragmentStart, fragmentEnd]
        ColumnIterator (Column &iterateMe, int fragmentStart, int fragmentEnd, int stepSize = COLUMN_ITERATOR_STEP);

        ColumnIterator ();
        // destructor... if there is a column left in the ColumnIterator, it will be
        // destroyed as well
        ~ColumnIterator ();

        // tells the iterator that we are done iterating... any new bytes written past
        // the end of the column will be included in the column from now on.  The column
        // is taken out of the iterator and put into iterateMe
        void Done (Column &iterateMe);

        // start from the beggining
        void Restart(void);

        // advance to the next object in the column...
        void Advance ();

        // add a new data object into the column at the current position, overwriting the
        // bytes that are already there.  Note that if the size of addMe differs from the
        // size of the object that is already there, addMe will over-run part of the next
        // object.  Sooooo... overwrite existing objects in the column with care!
        void Insert (const DataType &addMe);

        // returns true if the object under the cursor has never been written to and so
        // it should not be read (it is undefined what happens if you read it)
        int AtUnwrittenByte ();

        // returns the data object at the current position in the column...
        const DataType &GetCurrent ();

        /**
          create deep copy from 'fromMe'. Everything is created new down the hierarchy,
          basically all iterator states are replicated, new Column and MMappedStorage is
          created with partial data + states copied. Storage is write only for this iterator.
          The storage (data) is copied only up to the current position of fromMe. This is used
          (for example) in the join, where you find a tuple that matches with more than one tuple
          on the RHS and so now you need to start making multiple copies of values.

          Here is the short usage,
          ColumnIterator<Type> myIter;
        // make sure fromMe.Done(col) is NOT called before making deep copy
        // after below line, myIter is ready to add more data using Insert(), since
        // it is pointing at the end of new replicated partial storage.
        myIter.CreateDeepCopy (fromMe); // myIter will contain write only storage
        // and ready to add more data using Insert()
        // Now start Inserting into myIter
        */
        void CreateDeepCopy (ColumnIterator& fromMe);

        void CreateShallowCopy (ColumnIterator& fromMe);

        void CheckpointSave ();

        void CheckpointRestore ();

        // swap
        void swap (ColumnIterator& swapMe);

        void MarkFragment ();
};

template <class DataType, int headerSize, int dtSize >
inline const DataType &ColumnIterator <DataType, headerSize, dtSize > :: GetCurrent () {

    // For invalid columns this statement will turn out to be *((DataType*)NULL), this looks like
    // dereferencing a NULL ptr, but if you write like this, "const DataType& obj = iter.GetCurrent()",
    // it will not crash until you use obj. Compiler follows delayed dereferencing.
    Deserialize(it.GetData(), curItem);
    return curItem;
}

template <class DataType, int headerSize, int dtSize >
inline int ColumnIterator <DataType, headerSize, dtSize > :: AtUnwrittenByte () {
    return it.AtUnwrittenByte ();
}

template <class DataType, int headerSize, int dtSize >
inline void ColumnIterator <DataType, headerSize, dtSize > :: Insert (const DataType &addMe) {
    if (it.IsInvalid ()) return;
    assert (it.IsWriteOnly() == true);
    it.EnsureWriteSpace ();
    Serialize(it.GetData(), addMe);
}

template <class DataType, int headerSize, int dtSize >
inline void ColumnIterator <DataType, headerSize, dtSize > :: Advance () {
    it.Advance();
}


template <class DataType, int headerSize, int dtSize >
inline void ColumnIterator <DataType, headerSize, dtSize > :: Restart () {
    it.Restart();
}


template <class DataType, int headerSize, int dtSize >
inline void ColumnIterator <DataType, headerSize, dtSize > :: MarkFragment () {
    if (it.IsInvalid())
        return;
    it.MarkFragment ();
}

#endif
