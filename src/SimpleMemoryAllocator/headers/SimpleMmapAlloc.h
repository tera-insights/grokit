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
#ifndef _SIMPLE_MMAP_ALLOC_H_
#define _SIMPLE_MMAP_ALLOC_H_

#include <map>
#include <list>
#include <pthread.h>

#include "MmapAllocator.h"

#ifdef USE_HUGE_PAGES
#define HUGE_PAGE_FILE "/mnt/hugepages/alloc"
#endif

// number of large pages in the system
// we should read this from /proc/sys/vm/nr_hugepages instead of hardwireing
#define NUM_LPG_SYSTEM 60844

/** This header specifies the interface of the class SimpleMmapAlloc
 * and the implementation of mmap_alloc and mmap_free based on this class.

 * The class behaves like a singleton to ensure a single global allocator.

 * The strategy used is the following:
 * 1. We maintain a map from sizes (multiple of pages) to free lists
 * 2. When a request is made, we find the free list and return the first element
 * 3. If the list is empty, we use the system mmap to get an element

 * When the element is dealocated, we use the sizeMap to determine the size
 * The element is placed in the free list corresponding to the size once dealocated
 * If the pointer is not in the map, we signal an error

 This allocator is thread safe.

Note: This implementation is clearly a little wastefull but it is
simple. This is a thing that could be improved in the future.

*/

class SimpleMmapAlloc {
    pthread_mutex_t mutex; // to guard the implementation

#ifdef USE_HUGE_PAGES
    int hugePageFD;
    int numPgAlloc; // number of pages allocated
#endif

    /** struct to track elements of free lists */
    struct FreeListEl{
        void* ptr; // pointer to the free mem
        size_t timestamp; // timestamp that can be used to retire entries
        FreeListEl(void* _ptr, size_t _timestamp=0):
            ptr(_ptr), timestamp(_timestamp){ }
    };

    typedef std::map< int, list<FreeListEl>* > FreeListMap;
    FreeListMap freeListMap;// the map from sises to free lists
    // the size is expressed as a multiplier of the page size

    typedef std::map< void*, int > SizeMap; // size_t should be void* but
    // map does not like it
    SizeMap sizeMap; // the map from pointers to the size allocated (in pages)

    // translator from bytes to pages
    // rounds up the size
    int BytesToPageSize(size_t bytes);
    size_t PageSizeToBytes(int pSize);

    static SimpleMmapAlloc singleton;

    SimpleMmapAlloc(SimpleMmapAlloc&); // block the copy constructor

    public:
    // default constructor; initializes the allocator
    SimpleMmapAlloc(void);

    // function to get access to the singleton instance
    static SimpleMmapAlloc& GetAllocator(void);

    // functin to allocate
    void* MmapAlloc(size_t noBytes);

    // function to deallocate
    void MmapFree(void* ptr);

    // Destructor (frees the mmaps)
    virtual ~SimpleMmapAlloc(void);
};

// Inline functions
SimpleMmapAlloc& SimpleMmapAlloc::GetAllocator(void){
    return singleton;
}

#endif // _SIMPLE_MMAP_ALLOC_H_
