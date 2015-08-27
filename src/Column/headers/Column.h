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

#ifndef COL_H
#define COL_H

#include <pthread.h>
#include "RawStorageDesc.h"
#include "FileMetadata.h"
#include "DistributedCounter.h"
#include <functional>

class MMappedStorage;

/** Alin's changes: I broked the general implementation that depends on ColumnStorage and
    made everithing depend on MMappedStorage.

    To revert, do MMappedStorage->ColumnStorage in Column.h, ColumnPrivate.h and Column.cc
    and add a workable Swap for ColumnStorage
*/

// this is the basic column class... all columns hold a storage object that stores
// the actual data in the column
class Column {

public:
    typedef std::function<void(Column&)> Destroyer;

#include "ColumnPrivate.h"

public:

    // standard constructor and destructor
    Column ();
    ~Column ();

    // standard swap operator
    void swap (Column &withMe);

    // creates a column that contains the given storage... note that a shallow
    // copy of the storage is made and loaded into the Column, so that after
    // the call, loadMe can be loaded into another column if desired
    Column (MMappedStorage &loadMe, Destroyer _dest = Destroyer() );

    // interface to the storage functions


    // Compress the content and keep track of it internaly
    // compression happens in a single go (no incremental compression)
    // the storage state cannot be changed after this
    // if deleteDecompress is true, the decompressed version should be eliminated
    void Compress(bool deleteDecompressed);

    // give access to the compressed data to hte caller and put a
    // description of where the data is in "where". Returns the size in
    // bytes of the entire compressed data. If 0 returned, no compressed
    // version this function should not compress. The returned value
    // might be smaller than the sum of all pages described
    void GetCompressed(RawStorageList& where);
    off_t GetCompressedSizeBytes();
    off_t GetCompressedSizePages();
    bool GetIsCompressed();


    // access to uncompressed data. Should return the size of
    // uncompressed data even if where is not populated (in which case
    // the uncompresed version is not available)
    void GetUncompressed(RawStorageList& where);
    off_t GetUncompressedSizeBytes();
    off_t GetUncompressedSizePages();


    // makes a shallow copy of the column... for performance-related reasons,
    // this should NOT be sued when a swap would suffice, since a write when
    // you have multiple copies will trigger a copy-on-write
    void copy (Column &copyMe);

    // returns true if the column is not NULL (uninitialized)
    bool IsValid();

    // Create partial deep copy up to position given
    void CreatePartialDeepCopy (Column& fromMe, uint64_t curPosInColumn);

    // Get the mode (readonly or writeonly)
    bool IsWriteMode ();
    // make the column readonly. It should have been write only up to this point
    void MakeReadonly();
    
    // Give fragment handle as it may have several APIs in future for iterator use and not
    // needed to add all APIs in column
    Fragments& GetFragments ();
    void SetFragments (Fragments& _frag);
};

// Override global swap
void swap( Column & a, Column & b );

////////// INLINE FUNCTIONS ///////////////
inline bool Column::IsValid(){
    return (refCount != 0);
}

inline
void swap( Column & a, Column & b ) {
    a.swap(b);
}

#endif

