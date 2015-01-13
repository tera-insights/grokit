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

#ifndef STOR_UNIT_H
#define STOR_UNIT_H

#include <cstring>
#include <cstdlib>
#include "Errors.h"

#include <iostream>
#include "DistributedCounter.h" // REMOVE
#include "MmapAllocator.h"

// this structure is used by the simple mmapped storage to store all of its chunks of memory.
// Each chunk has a pointer to some mmapped storage, as well as a (logical) start and end
// position in the column where the chunk of memory fits
struct StorageUnit {

    // the actual data in the storage unit
    char *bytes;

    // the range of bytes that this unit covers (end is the last byte
    // that is actually included in the unit)
    int start;
    int end;
    DistributedCounter* refCount;

    // standard funcs to be able to use a two way list
    StorageUnit (char* _bytes = NULL, int _start = 0, int _end = 0): bytes(_bytes), start(_start), end(_end) {
        refCount = new DistributedCounter(1);
    }

    StorageUnit (const StorageUnit& other, int numaNode = NUMA_ALL_NODES) :
        bytes(nullptr),
        start(other.start),
        end(other.end),
        refCount(new DistributedCounter(1))
    {
        size_t size = (end - start) + 1; // range is inclusive
        bytes = (char *) mmap_alloc(size, numaNode);
        memcpy(bytes, other.bytes, size * sizeof(char));
    }

    void Clean() {
        if (refCount->Decrement(1) == 0){
            // last copy, delete everything
            delete refCount, refCount = NULL;
            if (bytes != NULL) {
                mmap_free(bytes);
                bytes = NULL;
            }
        }
    }

    void Kill(){}

    ~StorageUnit () {
        // deallocate everything if we are the only copy
        Clean();
    }

    int Size(void){ return end-start+1; }

    void swap (StorageUnit &withMe) {
        char storage[sizeof (StorageUnit)];
        memmove (storage, this, sizeof (StorageUnit));
        memmove (this, &withMe, sizeof (StorageUnit));
        memmove (&withMe, storage, sizeof (StorageUnit));
    }

    // does a simple copy
    void copy (StorageUnit &fromMe) {
        Clean();
        memmove (this, &fromMe, sizeof (StorageUnit));
        refCount->Increment(1);
    }

    // takes withMe and copies any overlapping content on top of the memory pointed to by *this
    void CopyOverlappingContent (StorageUnit &fromMe) {

        // find the overlapping range of bytes
        int firstByteToCopy = start;
        if (start < fromMe.start)
            firstByteToCopy = fromMe.start;

        int lastByteToCopy = end;
        if (end > fromMe.end)
            lastByteToCopy = fromMe.end;

        // if the range is empty, return
        if (firstByteToCopy > lastByteToCopy)
            return;

        // do the copy; memcpy is more efficient and there should be no overlap
        memcpy (bytes + firstByteToCopy - start, fromMe.bytes + firstByteToCopy - fromMe.start,
                lastByteToCopy - firstByteToCopy + 1);
    }
};

#endif
