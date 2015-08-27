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
#ifndef _DISK_ALLOC_H_
#define _DISK_ALLOC_H_

#include "Constants.h"
#include "sqlite3.h"

#include <map>
#include <vector>
#include <list>
#include <pthread.h>
#include <sys/mman.h>


// This may be weird value for now because origianlly when I set that, it was missing
// so setting anything to make it compile for now
#define DISK_ALLOC_BLOCK_SIZE 4096

class DiskMemoryAllocator {
    pthread_mutex_t mutex; // to guard the implementation

    // This manager is specific to disk array ID
    uint64_t mDiskArrayID;

    // Keeps track of global last free page of infinite capacity
    off_t mLastPage;

    // Keep the record of chunk
    struct ChunkInfo{
        off_t startPage;    // Keep start page number
        off_t size;         // keep the chunk size, for now this is always a block size
        off_t nextFreePage; // Next free page
    };

    // This maintains list of chunks per relation
    struct ChunkList {
        std::vector<ChunkInfo*> listOfChunks;
    };

    // relation ID pointing to list of chunks associated with this relation
    // last chunk will fill free page request
    std::map<uint64_t, ChunkList*> mRelationIDToChunkList;

    // Keep freed chunks for future use
    std::list<ChunkInfo*> mFreeChunks;

    DiskMemoryAllocator(DiskMemoryAllocator&); // block the copy constructor

    void CreateNewChunk(ChunkList* l);

    public:
    // default constructor; initializes the allocator
    DiskMemoryAllocator();

    // set the disk arrayID at a latter type
    void SetArrayID(uint64_t diskArrayID);

    // functin to allocate
    off_t DiskAlloc(off_t noPages, uint64_t relID);

    // function to deallocate complete relation
    void DiskFree(uint64_t relID);

    // method to switch the space from one relation to another relation
    // used to "glue" relations together
    void StealSpace(uint64_t oldRelID, uint64_t newRelID);

    void Flush(sqlite3* db);

    void Load(sqlite3* db);

    // How much space we are using
    off_t AllocatedSpace();

    // How much space is wasted
    off_t FragmentedSpace();

    // Destructor (frees the mmaps)
    ~DiskMemoryAllocator(void);
};

inline void DiskMemoryAllocator::SetArrayID(uint64_t diskArrayID){
    mDiskArrayID = diskArrayID;
}

#endif // _DISK_ALLOC_H_
