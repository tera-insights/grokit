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

#ifndef CI_CC
#define CI_CC

#include "ColumnIterator.h"
#include <iostream>
#include <assert.h>

#ifndef _dp_max
#define _dp_max(x,y) ((x)>(y)? (x) : (y))
#endif

template <class DataType, int headerSize, int dtSize >
ColumnIterator <DataType, headerSize, dtSize> :: ColumnIterator (Column &iterateMe, int stepSize ) : it (iterateMe, headerSize, _dp_max(stepSize, dtSize)) {

    // return if invalid column, because otherwise we set objLen and that is used in Advance unnecessarily to advance
    if (it.IsInvalid ())
        return;

    // Set the object size
    it.SetObjLen (dtSize);
}

template <class DataType, int headerSize, int dtSize >
ColumnIterator <DataType, headerSize, dtSize> :: ColumnIterator (Column &iterateMe, int fragmentStart, int fragmentEnd, int stepSize) :
    it (iterateMe, fragmentStart, fragmentEnd, headerSize, _dp_max(stepSize, dtSize)) {

        // return if invalid column, because otherwise we set objLen and that is used in Advance unnecessarily to advance
        if (it.IsInvalid ())
            return;

        // Set the object size
        it.SetObjLen (dtSize);
    }

template <class DataType, int headerSize, int dtSize>
ColumnIterator <DataType, headerSize, dtSize> :: ColumnIterator () : it () {
    // This is iterator with no column, hence invalid state set
}

template <class DataType, int headerSize, int dtSize>
ColumnIterator <DataType, headerSize, dtSize> :: ~ColumnIterator () {
}

template <class DataType, int headerSize, int dtSize>
void ColumnIterator <DataType, headerSize, dtSize> :: Done (Column &iterateMe) {
    it.Done(iterateMe);
}

template <class DataType, int headerSize, int dtSize>
void ColumnIterator <DataType, headerSize, dtSize> :: CreateDeepCopy (ColumnIterator<DataType, headerSize, dtSize>& fromMe) {
    it.CreateDeepCopy(fromMe.it);
}

template <class DataType, int headerSize, int dtSize>
void ColumnIterator <DataType, headerSize, dtSize> :: swap (ColumnIterator& swapMe) {
    it.swap (swapMe.it);
}

template <class DataType, int headerSize, int dtSize>
void ColumnIterator <DataType, headerSize, dtSize> :: CreateShallowCopy (ColumnIterator& copyMe) {
    it.CreateShallowCopy (copyMe.it);
}

template <class DataType, int headerSize, int dtSize>
void ColumnIterator <DataType, headerSize, dtSize> :: CheckpointSave () {
    it.CheckpointSave ();
}

template <class DataType, int headerSize, int dtSize>
void ColumnIterator <DataType, headerSize, dtSize> :: CheckpointRestore () {
    it.CheckpointRestore ();
}

#endif
