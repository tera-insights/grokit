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
#ifndef CHUNK_H
#define CHUNK_H

#include "ChunkID.h"
#include "QueryID.h"
#include "TableScanID.h"
#include "QueryExit.h"
#include "Column.h"
#include "BStringIterator.h"

// Special slot reservations. Only Chunk is influenced by this
// If the number of slots is increased, the macro FIRST_NONRESERVED_SLOT in AttributeManager.h
// should be increased
// make sure the slot allocation is made in order to avoid waistage
#define BITSTRING_SLOT 0
#define HASH_SLOT 1

class Chunk {

#include "ChunkPrivate.h"

    public:
        // constructor
        Chunk ();

        // destructor
        virtual ~Chunk ();

        // This class encapsulates the data that moves around the system.

        // This swaps the column into/out of the chunk
        void SwapColumn (Column &addMe, int pos);

        // This swaps the bitmap into the chunk
        void SwapBitmap (BStringIterator &swapMe);

        // This swaps the column of hash values into the chunk
        void SwapHash (Column &addMe);

        void CreateBitstringFromCol (Column& col);

        // make the chunk readonly
        void MakeReadonly();

        // This copies the chunk.  The copy is generally a fast, shallow
        // one (Column.copy is called for each of the columns in the chunk,
        // and since columns are read-only, Column.copy is shallow and fast).
        // However, the copy of the bitmap is a deep one.  That is, after
        // calling copy, one can read/write the bitmap and there will be
        // no aliasing issues
        void copy (Chunk &copyMe);

        // this swaps the contents of two chunks... this is the preferred
        // way to move chunks around
        void swap (Chunk &swapMe);

        //Returns number of columns
        int GetNumOfColumns();
        int GetNumTuples();

};

// This name is little different because ChunkContainer name is already occupied by ExecEngineData.h
typedef TwoWayList<Chunk>  ContainerOfChunks;

// Overload global swap
void swap( Chunk & a, Chunk & b );

#endif
