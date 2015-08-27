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

#include "Errors.h"
#include "DistributedCounter.h" // REMOVE
#include "MmapAllocator.h"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <utility>

// this structure is used by the simple mmapped storage to store all of its chunks of memory.
// Each chunk has a pointer to some mmapped storage, as well as a (logical) start and end
// position in the column where the chunk of memory fits
struct StorageUnit {

    // the actual data in the storage unit
    char *bytes;

    // the range of bytes that this unit covers (end is the last byte
    // that is actually included in the unit)
    uint64_t start;
    uint64_t end;
    DistributedCounter* refCount;

public:
    // standard funcs to be able to use a two way list
    StorageUnit (char* _bytes = nullptr, uint64_t _start = 0, uint64_t _end = 0):
        bytes(_bytes),
        start(_start),
        end(_end),
        refCount(new DistributedCounter(1))
    {

    }

    StorageUnit (const StorageUnit& other, uint64_t numaNode = NUMA_ALL_NODES) :
        bytes(nullptr),
        start(other.start),
        end(other.end),
        refCount(new DistributedCounter(1))
    {
        size_t size = (end - start) + 1; // range is inclusive
        bytes = (char *) mmap_alloc(size, numaNode);
        memcpy(bytes, other.bytes, size * sizeof(char));
    }

    void MakeReadonly(){
        mmap_prot_read(bytes);
    }

    void Clean() {
        if (refCount->Decrement(1) == 0){
            // last copy, delete everything
            delete refCount;
            refCount = nullptr;
            if (bytes != nullptr) {
                mmap_free(bytes);
                bytes = nullptr;
            }
        }
    }

    void Kill(){}

    ~StorageUnit () {
        // deallocate everything if we are the only copy
        Clean();
    }

    uint64_t Size(void){ return end-start+1; }

    void swap (StorageUnit &other) {
        using std::swap;

        swap(bytes, other.bytes);
        swap(start, other.start);
        swap(end, other.end);
        swap(refCount, other.refCount);
    }

    // does a simple copy
    void copy (StorageUnit &fromMe) {
        Clean();

        bytes = fromMe.bytes;
        start = fromMe.start;
        end = fromMe.end;
        refCount = fromMe.refCount;

        refCount->Increment(1);
    }

    // takes withMe and copies any overlapping content on top of the memory pointed to by *this
    void CopyOverlappingContent (StorageUnit &fromMe) {

        // find the overlapping range of bytes
        uint64_t firstByteToCopy = start;
        if (start < fromMe.start)
            firstByteToCopy = fromMe.start;

        uint64_t lastByteToCopy = end;
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

inline
void swap(StorageUnit& a, StorageUnit& b) {
    a.swap(b);
}

#endif
