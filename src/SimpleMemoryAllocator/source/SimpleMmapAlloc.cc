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
#include <sys/mman.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Numa.h"
#include "Errors.h"
#include "SimpleMmapAlloc.h"

/** Changelog: Alin 11/04/2009

		The OS is not giving an error when we allocate a large page. We
		need to do our own counding and to tap into the main memory pool
		if we run out of large pages
*/


/** Changelog:
		Alin changed this to use the hugepages so that it can be used
		with the hash tables as well as with the disk

		Variable USE_HUGE_PAGES has to be defined. Define it in the
		Makefile if you want to use the facility.
*/


SimpleMmapAlloc SimpleMmapAlloc::singleton;

void* mmap_alloc(size_t noBytes, int node){
	// in this implementatin we ignore the numa hint
	SimpleMmapAlloc& aloc=SimpleMmapAlloc::GetAllocator();
	return aloc.MmapAlloc(noBytes);
}

void mmap_free(void* ptr){
	SimpleMmapAlloc& aloc=SimpleMmapAlloc::GetAllocator();
	aloc.MmapFree(ptr);
}

SimpleMmapAlloc::SimpleMmapAlloc(void){
	// the maps are automatically created
	// initialize the mutex
	pthread_mutex_init(&mutex, NULL);

#ifdef USE_HUGE_PAGES
	numPgAlloc=0;
	hugePageFD =  open(HUGE_PAGE_FILE, O_CREAT | O_RDWR, 0755);
	FATALIF(hugePageFD < 0, "Could not open %s.\nDid you mount /mnt/hugepages?\n",
		HUGE_PAGE_FILE);
#endif

}

int SimpleMmapAlloc::BytesToPageSize(size_t bytes){
	// compute the size in pages
	int pSize = bytes >> ALLOC_PAGE_SIZE_EXPONENT;
	if (bytes != PageSizeToBytes(pSize) )
		pSize++; // extra page to get the overflow

	return pSize;
}

size_t SimpleMmapAlloc::PageSizeToBytes(int pSize){
	return ((size_t) pSize) << ALLOC_PAGE_SIZE_EXPONENT;
}

void* SimpleMmapAlloc::MmapAlloc(size_t noBytes){
	if (noBytes == 0)
		return NULL;

	pthread_mutex_lock(&mutex);

	int pSize = BytesToPageSize(noBytes);

	// find the list for this size
	FreeListMap::iterator it=freeListMap.find(pSize);
	list<FreeListEl>* listPtr;
	if (it==freeListMap.end()){
		// we did not find the list; create it
		listPtr = new list<FreeListEl>;
		freeListMap.insert(pair<int, list<FreeListEl>*>(pSize, listPtr));

		cerr << "ALLOCATOR: created list for size " << pSize << endl;
	} else {
		// we found the list
		listPtr = (*it).second;
	}
	// we have the list, see if we have any element
	void * rezPtr; // the result pointer
	if (listPtr->empty()){
		// we do not, allocate the memory using mmap
#ifdef USE_HUGE_PAGES
		// did we allocate all the large pages?
		if (numPgAlloc+pSize>NUM_LPG_SYSTEM){
			// do a regular mmap and pray it is good enough
			rezPtr = mmap(NULL, PageSizeToBytes(pSize),
				PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);
			cout << "Allocated from memory %d pages" << pSize << endl;
			cout << "We tried to allocate " << pSize << " pages and numPgAlloc=" << numPgAlloc << endl;
		} else {
			// have enough
			numPgAlloc+=pSize;
			rezPtr = mmap(NULL, PageSizeToBytes(pSize),
				PROT_READ | PROT_WRITE, MAP_PRIVATE, hugePageFD, 0);
		}
#else
		rezPtr = mmap(NULL, PageSizeToBytes(pSize),
			PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);
#endif
		FATALIF( rezPtr == MAP_FAILED, "MMap failed, no more memory.");
	} else {
		// we have the element
		rezPtr = (listPtr->front()).ptr;
		listPtr->pop_front(); // eliminate the front
	}

	// record the allocation in sizeMap
	sizeMap.insert(pair<void*, int>(rezPtr, pSize));

	//	cerr << "ALLOC:" << rezPtr << " of size " << pSize << " mapSize " << sizeMap.size() << endl;

	pthread_mutex_unlock(&mutex);

	return rezPtr;
}


void SimpleMmapAlloc::MmapFree(void* ptr){
	if (ptr==NULL)
		return;

	pthread_mutex_lock(&mutex);

	assert (sizeMap.size()!=0);

	// find the size and insert the freed memory in the
	SizeMap::iterator it=sizeMap.find(ptr);
	FATALIF(it==sizeMap.end(), "Deallocating unallocated pointer %p.", ptr); // if we do not find it, it is bad
	int pSize=it->second;
	// delete the element from the sizeMap.
	sizeMap.erase(it);

	// put this pointer into the free list
	FreeListMap::iterator it2=freeListMap.find(pSize);
	assert(it2!=freeListMap.end());
	list<FreeListEl>* listPtr=it2->second;
	FreeListEl lEl(ptr);
	listPtr->push_front(lEl);

	//	cerr << "FREED:" << ptr << " of size " << pSize << endl;

	pthread_mutex_unlock(&mutex);
}


SimpleMmapAlloc::~SimpleMmapAlloc(void){
	// dealocate the mutex
	pthread_mutex_destroy(&mutex);
	// it would be nice to deallocate the memory with munmap as well
}
