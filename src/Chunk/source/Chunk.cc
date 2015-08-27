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
#include <stdio.h>
#include <string.h>
#include <vector>

#include "Chunk.h"
#include "Errors.h"

using namespace std;

Chunk :: Chunk () {
    cols = NULL;
    numCols = 0;
    actualNumCols = 0;
}


Chunk :: ~Chunk () {
    for (int i = 0; i < numCols; i++) {
        delete cols[i], cols[i]=NULL;
    }

    delete [] cols, cols=NULL;
    numCols = 0, actualNumCols = 0;
}

void Chunk :: SwapHash (Column &addMe) {
  SwapColumn (addMe, HASH_SLOT);
}

void Chunk :: SwapBitmap (BStringIterator &swapMe) {
  // first make sure that mbitColumn is ready.
  // If the data was read from the disk, the BitString is in the
  // BITSTRING_SLOT and first needs to be converted so it
  // behaves like the iterator
  // Once this process is done ONCE, the column in BISTRING_SLOT
  // is empty, and will not change the iterator

  //Column col;
  //SwapCoxolumn(col, BITSTRING_SLOT);
  //mbitColumn.ConvertFromCol(col);

  // now swap
  mbitColumn.swap (swapMe);
}

void Chunk :: CreateBitstringFromCol (Column& col) {
    mbitColumn.ConvertFromCol(col);
}

void Chunk :: SwapColumn (Column &addMe, int pos) {

    // if we are swapping a column past the end of the chunk, get the new space
    if (pos >= numCols) {
        Column **temp = new Column *[(pos + 1) * 2];

        // then put the old data in
        for (int i = 0; i < numCols; i++) {
            temp[i] = cols[i];
        }

        // and zero out the new slots
        for (int i = numCols; i < (pos + 1) * 2; i++) {
            temp[i] = 0;
        }

        // and put the new list of columns in
        delete [] cols;
        cols = temp;
        numCols = (pos + 1) * 2;
    }

    // now, do the slot.  First see if it is a zero... if it is, we need to
    // allocate it
    if (cols[pos] == 0) {
        cols[pos] = new Column;
        actualNumCols += 1;
    }

    // and swap the column in
    cols[pos]->swap (addMe);
}

void Chunk :: copy (Chunk &copyMe) {

    // first, swap this guy for an empty one
    Chunk empty;
    swap (empty);

    // copy over the bitmap
    mbitColumn.copy (copyMe.mbitColumn);

    // and copy over the columns
    actualNumCols = copyMe.actualNumCols;
    numCols = copyMe.numCols;
    cols = new Column*[numCols];
    for (int i = copyMe.numCols - 1; i >= 0; i--) {
        // if this column is used, copy it
        if (copyMe.cols[i] != 0) {
            cols[i] = new Column;
            cols[i]->copy(*(copyMe.cols[i]));
        } else {
            cols[i] = NULL; // big bug otherwise
        }
    }
}

void Chunk :: swap (Chunk &swapMe) {

    SWAP_ASSIGN(cols, swapMe.cols);
    SWAP_STD(numCols, swapMe.numCols);
    SWAP_STD(actualNumCols, swapMe.actualNumCols);
    mbitColumn.swap(swapMe.mbitColumn);
    /*
       char *foo = new char[sizeof (Chunk)];
       memmove (foo, this, sizeof (Chunk));
       memmove (this, &swapMe, sizeof (Chunk));
       memmove (&swapMe, foo, sizeof (Chunk));
       delete [] foo;
*/
}

// overload global swap
void swap( Chunk & a, Chunk & b ) {
    a.swap(b);
}

int Chunk :: GetNumOfColumns() {
    return actualNumCols;
}

int Chunk :: GetNumTuples() {

    return mbitColumn.GetNumTuples();
}


void Chunk :: MakeReadonly(){
  // scans all the columns and make them readonly
  for (int i = numCols - 1; i >= 0; i--) {
        // if this column is used, copy it
        if (cols[i] != 0) {
            cols[i] -> MakeReadonly();
          } 
    }
}