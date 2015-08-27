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
#ifndef _NUMA_MMAP_ALLOC_H_
#define _NUMA_MMAP_ALLOC_H_

#include <map>
#include <set>
#include <list>
#include <vector>
#include <pthread.h>

#include "MmapAllocator.h"
// Below 3 headers need for constant used for defining fixed hash size HASH_SEG_SIZE
#include "HashTableMacros.h"
#include "Constants.h"
#include "HashEntry.h"

/* This enables to store header information in chunk itself. If not set, header
   info will be stored elsewhere. But it turns out, we can not use this flag
   because it will give us pointers which are not aligned to the page boundary
   and hence disk writing will fail.
   */
//#define STORE_HEADER_IN_CHUNK 1

// For debug purpose
#define ASSERT_ON 1

#ifndef ASSERT
#ifdef ASSERT_ON
#define ASSERT(expr) assert(expr)
#else
#define ASSERT(expr)
#endif
#endif

/** This header specifies the interface of the class NumaMemoryAllocator
 * and the implementation of mmap_alloc and mmap_free based on this class.

 * The class behaves like a singleton to ensure a single global allocator.

 * The strategy used is the following:
 * 1. We initialize the heap for all numa nodes
 * 2. We serve requests from allocated heap (smaller chunks are carved out
 *    from biggest available chunk) if requested chunk is not exact match in freelist
 * 3. We maintain a map from sizes (multiple of pages) to free lists per numa node
 * 5. When chunk is freed, it is immediately coalesced with neighbouring chunks to reduce fragmentation
 * 6. With each chunk, a header information is maintained which helps in coalescing of chunks
 *    If header information is maintained within chunk itself, super fast! because
 *    we dont need to lookup header chunk using pointer given from ptr->header map
 * 7. When requested chunk is not found in numa, we check other numa nodes
 * 8. When requested size is not found in freelist, we pick biggest chunk, it is
 *    no time operation as we take out last element of map (sorted by size) and
 *    if it has element bigger than requested size, it will have freelist too for sure.
 * 9. Coelasceing of chunks is just some assignment of pointers if header is stored in
 *    chunk itself.
 * 10. Splitting of chunks is also some assignment of pointers if header is stored in
 *     chunks. And if not stored within chunk, we have to pay little search penalty.
 * 11. Header is 40 bytes long (void*) aligned.

 This allocator is thread safe.

*/

// Up to this number, no merging of adjacent chunks
#define NO_COALESCE_MAXPAGESIZE 16

// This is special size for hash segments and handled differently
#define HASH_SEG_SIZE (ABSOLUTE_HARD_CAP * sizeof(HashEntry))

// Touch the pages once retreived from mmap.
#define MMAP_TOUCH_PAGES 1

// Initial heap size for all NUMA nodes
#define INIT_HEAP_PAGE_SIZE 256*4

// Grow heap during run by this size if needed
#define HEAP_GROW_BY_SIZE 256*16

// This checks memory leaks.
class MemoryCheck {

    struct Info {
        const char* filename;
        int linenum;
        size_t size;
    };
    typedef std::map<void*, Info*> PtrMap;
    PtrMap ptrToInfoMap;

    public:
    MemoryCheck();

    void Insert(void* ptr, size_t _size, const char* _filename, int _linenum);

    void Delete(void* ptr);

    void Print();
};

class NumaMemoryAllocator {

    pthread_mutex_t mutex; // to guard the implementation

#ifdef MMAP_CHECK
    MemoryCheck memChk;
#endif


    bool mHeapInitialized;
    struct NumaNode; // forward declaration

    /* Keep the record of all chunks, allocated and unallocated both (for coalesce)
       This contains the header information per allocated chunk (using mmap_alloc)
       This header will be used to merge two free chunks to create bigger chunk to reduce
       fragmentation
       */
    struct ChunkInfo{
        ChunkInfo* prevChunk; // pointer to previous physical chunk
        ChunkInfo* nextChunk; // pointer to next physical chunk
        void* currentPtr;     // Keep current pointer too for ease of code
        NumaNode* numaNode;

        union SizeInfo{
            struct SizeStruct{
                int size; // size of current chunk in pages, dont use size_t
                bool isFree; // Is current chunk free?
            };
            SizeStruct sizeStruct;
            void* Align;
        };

        SizeInfo sizeInfo; // void* aligned
    };

    // This keeps track of all freelists per numa node
    struct NumaNode{
        std::map<int, std::set<void*>*> mSizeToFreeListMap;
    };

    // map to keep track of allocated data to verify double free error
    typedef std::map< void*, int > SizeMap;
    SizeMap sizeMap; // the map from pointers to the size allocated (in pages)


#ifndef STORE_HEADER_IN_CHUNK
    std::map<void*, ChunkInfo*> mPtrToChunkInfoMap;

    // Avoid new and delete, keep deletes chunks for future use.
    std::list<ChunkInfo*> mDeletedChunks;
#endif
    // Avoid new and delete, keep deleted lists for future use.
    std::list<std::set<void*>*> mDeletedLists;

    // Hash for numa number to numa pointer, speed up lookup
    std::vector<NumaNode*> mNumaNumberToNumaNode;

    // special list for fixed size hash segments, no relation to other guys
    std::set<void*> fixedSizeList;
    std::set<void*> fixedSizeOccupiedList;

    // translator from bytes to pages
    // rounds up the size
    int BytesToPageSize(size_t bytes);
    size_t PageSizeToBytes(int pSize);

    //static NumaMemoryAllocator& singleton();

    NumaMemoryAllocator(NumaMemoryAllocator&); // block the copy constructor

    // Update our freelist, either in case of merging chunks, allocating memory or
    // freeing memory operations
    void UpdateFreelist(ChunkInfo* chunkInfo);

    // If chunk is allocated, it is not free anymore. Remove from our freelist
    void RemoveFromFreelist(ChunkInfo* chunkInfo);

    // This merges two adjacent free chunks. Adjacent chunks would mean they must
    // be adjacent in physical memory
    void Coalesce(void* ptr);

    // This initializes the heap (for all nodes in case of NUMA) in very first call of mmap_alloc
    void HeapInit();

    // Helper function to reduce same code at multiple places
    void SearchFreeList(NumaNode*, int pSize, bool& exactListFound, bool& biggerListFound,
            std::map<int, std::set<void*>*>::iterator& iter,
            std::map<int, std::set<void*>*>::reverse_iterator& r_iter);

    // Helper function to reduce same code at multiple places
    void SearchFreeListSmallestFirst(NumaNode*, int pSize, bool& exactListFound, bool& biggerListFound,
            std::map<int, std::set<void*>*>::iterator& iter);

    public:
    // default constructor; initializes the allocator
    NumaMemoryAllocator(void);

    // function to get access to the singleton instance
    static NumaMemoryAllocator& GetAllocator(void);

    // functin to allocate
    void* MmapAlloc(size_t noBytes, int numaNode = 0, const char* file = NULL, int line=-1);

    // change the protection to prot: must be a combination of PROT_READ and PROT_WRITE
    void MmapChangeProt(void* ptr, int prot);

    // function to deallocate
    void MmapFree(void* ptr);

    // function to get size of allocated memory
    int SizeAlloc(void* ptr);

    // Tells the size which is still allocated
    size_t AllocatedPages();
    size_t FreePages();

#ifdef MMAP_CHECK
    void Diagnose();
#endif

    // Destructor (frees the mmaps)
    ~NumaMemoryAllocator(void);
};

// To avoid static initialization order fiasco. This is needed only if our allocator
// is used to initialize some global or static objects, where the ordering of
// initialization is undefined. This will help to fix such issues. Besides, if we know
// we don't have any such usage, static object can be defined outside this function
// and can be used directly. But to be safe, it's good this way
inline
NumaMemoryAllocator& NumaMemoryAllocator::GetAllocator(void){
    static NumaMemoryAllocator* singleton = new NumaMemoryAllocator();
    return *singleton;
}

#endif // _BIGCHUNK_MMAP_ALLOC_H_
