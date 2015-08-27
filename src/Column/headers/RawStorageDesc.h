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
#ifndef RAW_STORAGE_DESC_H_
#define RAW_STORAGE_DESC_H_

#include "Swap.h"
#include "TwoWayList.cc"
//#include "NumaMemoryAllocator.h" // sarvesh: uncomment

/** This is used as a container class to describe raw storage so that
  we have a fast interface between the memory and disk data structures

  This should not be used by any other component than the disk and
  network since it breaks the swapping paradigm. It effectively
  allows the memory to be referred to in two ways.

*/

class RawStorageUnit {
public:
    void* data;
    // This is size in pages. For last page, there may be some unused space
    uint64_t sizeInPages;

    // Size in bytes is added for testing purpose only, and can be remove later
    uint64_t sizeInBytes;

    // For compatibility for TwoWayList
    RawStorageUnit() {}

    RawStorageUnit(void* _data, uint64_t _pages, uint64_t _bytes):
        data(_data), sizeInPages(_pages), sizeInBytes(_bytes) {}

    void swap(RawStorageUnit& o){
        SWAP_ASSIGN(data, o.data);
        SWAP_STD(sizeInPages, o.sizeInPages);
        SWAP_STD(sizeInBytes, o.sizeInBytes);
    };

    void Kill () {
        //if (data)
        // mmap_free (data); // sarvesh, uncomment later
        data = 0;
    }

};

/** the whole request. It  is assumed that the large request is the
  concatenation IN ORDER of the small requests. */

typedef TwoWayList<RawStorageUnit> RawStorageList;

// overload global swap
inline
void swap( RawStorageUnit & a , RawStorageUnit & b ) {
    a.swap(b);
}

#endif // RAW_STORAGE_DESC_H_
