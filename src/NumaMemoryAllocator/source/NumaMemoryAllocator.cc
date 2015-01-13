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

#include "MmapAllocator.h"
#include "Errors.h"
#include "Numa.h"
#include "NumaMemoryAllocator.h"

using namespace std;

void* mmap_alloc_imp(size_t noBytes, int node, const char* f, int l){
	NumaMemoryAllocator& aloc=NumaMemoryAllocator::GetAllocator();
	void* rez= aloc.MmapAlloc(noBytes, node, f, l);
	return rez;
}

void mmap_free_imp(void* ptr, const char* f, int l){
    if( ptr == NULL ) {
        WARNING("Warning: Attempted free of null pointer at %s:%d", f, l);
    }

	NumaMemoryAllocator& aloc=NumaMemoryAllocator::GetAllocator();
	aloc.MmapFree(ptr);
}

off_t mmap_used(void){
	NumaMemoryAllocator& aloc=NumaMemoryAllocator::GetAllocator();
	return PAGES_TO_BYTES(aloc.AllocatedPages());
}


void mmap_diagnose() {
#ifdef MMAP_CHECK
	NumaMemoryAllocator& aloc=NumaMemoryAllocator::GetAllocator();
	aloc.Diagnose();
#endif
}

MemoryCheck::MemoryCheck() {
}


void MemoryCheck::Insert(void* ptr, size_t _size, const char* _filename, int _linenum) {
	PtrMap::iterator it = ptrToInfoMap.find(ptr);
	FATALIF(it!=ptrToInfoMap.end(), "Pointer %p already exists", ptr);
	FATALIF(_filename == NULL, "Filename received is NULL");
	FATALIF(_linenum == -1, "Linenumber is -1");
	Info* st = new Info;
	st->filename = _filename;
	st->linenum = _linenum;
	st->size = _size;
	ptrToInfoMap[ptr] = st;
}

void MemoryCheck::Delete(void* ptr) {
	PtrMap::iterator it = ptrToInfoMap.find(ptr);
	FATALIF(it==ptrToInfoMap.end(), "Pointer %p not found, already deleted or never added", ptr);
	delete it->second;
	ptrToInfoMap.erase(it);
}

	struct Temp {
		const char* fileN;
		size_t sz;
	};
void MemoryCheck::Print() {
	printf("\n Below are memory which are not deleted");
	map<int, Temp> lineToFile;
	for (PtrMap::iterator it = ptrToInfoMap.begin(); it != ptrToInfoMap.end(); it++) {
		map<int, Temp>::iterator i = lineToFile.find((it->second)->linenum);
		if (i == lineToFile.end()) {
			Temp t;
			t.fileN = it->second->filename;
			t.sz = it->second->size;
			lineToFile[(it->second)->linenum] = t;
		} else {
			FATALIF(it->second->filename != i->second.fileN, "Case not handled yet, fix later");
			i->second.sz += it->second->size;
		}
	}
	for (map<int, Temp>::iterator it = lineToFile.begin(); it != lineToFile.end(); it++) {
		printf("\n filename = %s, lineNum = %d, total size in pages = %ld", it->second.fileN, it->first, BYTES_TO_PAGES(it->second.sz));
	}
	printf("\n");
}

NumaMemoryAllocator::NumaMemoryAllocator(void)
{
	// initialize the mutex
	pthread_mutex_init(&mutex, NULL);
	mHeapInitialized = false;
}


int NumaMemoryAllocator::BytesToPageSize(size_t bytes){
	// compute the size in pages
	int pSize = bytes >> ALLOC_PAGE_SIZE_EXPONENT;
	if (bytes != PageSizeToBytes(pSize) )
		pSize++; // extra page to get the overflow

	return pSize;
}

size_t NumaMemoryAllocator::PageSizeToBytes(int pSize){
	return ((size_t) pSize) << ALLOC_PAGE_SIZE_EXPONENT;
}

void NumaMemoryAllocator::HeapInit()
{
	mHeapInitialized = true;
#ifndef USE_NUMA
	int numNumaNodes = 1;
#else
	int numNumaNodes = numaNodeCount();
#endif
	unsigned long nodeMask = 0;
	for (unsigned long long node = 0; node < numNumaNodes; node++)
	{
		nodeMask = 0;
		nodeMask |= (1 << node);
		NumaNode* numa = new NumaNode;
		//mNumaNumberToNumaNodeMap[node] = numa;
		mNumaNumberToNumaNode.push_back(numa);
		int pageFD = 0;
		void* newChunk = SYS_MMAP_ALLOC( PageSizeToBytes(INIT_HEAP_PAGE_SIZE) );
		if (!SYS_MMAP_CHECK(newChunk)){
			perror("NumaMemoryAllocator");
			FATAL("The memory allocator could not allocate memory");
		}
#ifdef USE_NUMA
#ifdef MMAP_TOUCH_PAGES
		// now bind it to the node and touch all the pages to make sure memory is bind to the node
		int retVal = mbind(newChunk, PageSizeToBytes(INIT_HEAP_PAGE_SIZE), MPOL_PREFERRED, &nodeMask, numNumaNodes+1, MPOL_MF_MOVE);
		ASSERT(retVal == 0);
		int* pInt = (int*)newChunk;
		for (unsigned int k = 0; k < PageSizeToBytes(INIT_HEAP_PAGE_SIZE)/4; k+=(1<<(ALLOC_PAGE_SIZE_EXPONENT-2)))
		pInt[k] = 0;
#endif
#endif

		// Below code fills the header information in our newly created biggest chunk of infinite size
		// We also must update freelist about this chunk, that it is free right now
#ifndef STORE_HEADER_IN_CHUNK
		ChunkInfo* chunk = NULL;
		if (mDeletedChunks.empty())
			chunk = new ChunkInfo;
		else
		{
			chunk = mDeletedChunks.front();
			mDeletedChunks.pop_front();
		}
		chunk->currentPtr = newChunk;
		// add new chunk entry in map
		mPtrToChunkInfoMap[chunk->currentPtr] = chunk;
#else
		// Note that this header eats up few bytes from our free chunk
		ChunkInfo* chunk = (ChunkInfo*)newChunk;
		chunk->currentPtr = (void*)((char*)newChunk + sizeof(ChunkInfo));
#endif
		chunk->prevChunk = NULL;
		chunk->nextChunk = NULL;
		chunk->numaNode = numa;
		(chunk->sizeInfo).sizeStruct.size = INIT_HEAP_PAGE_SIZE; // IMPORTANT, removed last 1 page so that we dont advance further in unmapped region
		(chunk->sizeInfo).sizeStruct.isFree = true;
		UpdateFreelist(chunk);
	}
}

/* For given numa node, we need to search our freelist for given size of request, we may find exact match
// in some of our freelist but if not, we always have bigger freelist to serve the request from. Below
// strategy uses best fit method. Essentially we have sorted list, so we can also say first fit.
*/
void NumaMemoryAllocator::SearchFreeListSmallestFirst(NumaNode* n, int pSize, bool& exactListFound, bool& biggerListFound,
map<int, set<void*>*>::iterator& iter)
{
	exactListFound = false;
	biggerListFound = false;
	iter = n->mSizeToFreeListMap.find(pSize);
	if (iter != n->mSizeToFreeListMap.end())
		exactListFound = true;
	else
	{
    /* When we did not find exact matching chunk size, we look for best available chunk to slice from
		// We have to pay little penalty to search for the best fit, but penalty is small as we have sorted
		// list, hence first chunk which fits the request, is the best one.
		*/
		for (iter = n->mSizeToFreeListMap.begin(); iter != n->mSizeToFreeListMap.end(); ++iter)
		{
			if (iter->first > pSize)
			{
				biggerListFound = true;
				break;
			}
		}
	}
}

/* For given numa node, we need to search our freelist for given size of request, we may find exact match
// in some of our freelist but if not, we always have bigger freelist to serve the request from. Below
// strategy uses worst fit method. Essentially we have sorted list, so we take out biggest chunk to cut
// from the end.
*/
void NumaMemoryAllocator::SearchFreeList(NumaNode* n, int pSize, bool& exactListFound, bool& biggerListFound,
map<int, set<void*>*>::iterator& iter,
map<int, set<void*>*>::reverse_iterator& r_iter)
{
	exactListFound = false;
	biggerListFound = false;
	iter = n->mSizeToFreeListMap.find(pSize);
	if (iter != n->mSizeToFreeListMap.end())
		exactListFound = true;
	else
	{
    /* When we did not find exact matching chunk size, we look for biggest available chunk to slice from
    // We are looking for biggest, hence we start from the end, it should match in very first iteration
    // hence no loop required to check other lesser bigger chunks since our map only have information about
    // free chunks. If not free, information for that chunk wouldn't really be here in our mSizeToFreeListMap
    // Hence the last element is our choice. This helps to reduce loop time
		*/
		r_iter = n->mSizeToFreeListMap.rbegin();
		if (r_iter != n->mSizeToFreeListMap.rend() && r_iter->first >= pSize)
		biggerListFound = true;
	}
}

void* NumaMemoryAllocator::MmapAlloc(size_t noBytes, int node, const char* f, int l){
	if (noBytes == 0)
		return NULL;

	pthread_mutex_lock(&mutex);

	//printf("\n Allocated size = %ld, Free pages size = %ld", AllocatedPages(), FreePages()); fflush(stdout);

	if (!mHeapInitialized)
	HeapInit();
#ifndef USE_NUMA
	node = 0;
#endif
	NumaNode* numa = NULL;
	numa = mNumaNumberToNumaNode[node];
	ASSERT(numa);
	//map<int, NumaNode*>::iterator itNuma= mNumaNumberToNumaNodeMap.find(node);
	//ASSERT (itNuma != mNumaNumberToNumaNodeMap.end());
	//numa = itNuma->second;

#ifndef STORE_HEADER_IN_CHUNK
	int pSize = BytesToPageSize(noBytes);
#else
	int pSize = BytesToPageSize(noBytes+sizeof(ChunkInfo));
#endif

	int hash_seg_size = BytesToPageSize(HASH_SEG_SIZE);
	if (pSize == hash_seg_size) {
		if (fixedSizeList.empty()) {
			//int pageFD = 0;
			void* newChunk = SYS_MMAP_ALLOC( PageSizeToBytes(hash_seg_size) );
			if (!SYS_MMAP_CHECK(newChunk)){
				perror("NumaMemoryAllocator");
				FATAL("The memory allocator could not allocate memory");
			}
			fixedSizeOccupiedList.insert(newChunk);
			pthread_mutex_unlock(&mutex);
			return newChunk;
		}
		set<void*>::iterator is = fixedSizeList.begin();
		void* res = (*is);
		fixedSizeList.erase(is);
		fixedSizeOccupiedList.insert(res);
		pthread_mutex_unlock(&mutex);
		return res;
	}

	bool exactListFound = false;
	bool biggerListFound = false;
	//map<int, set<void*>*>::reverse_iterator r_iter;
	map<int, set<void*>*>::iterator iter;

	/* Search freelist in given NUMA, either we will have exact match or we may find bigger
		 chunk to slice from
	*/
	//SearchFreeList(numa, pSize, exactListFound, biggerListFound, iter, r_iter);
	SearchFreeListSmallestFirst(numa, pSize, exactListFound, biggerListFound, iter);

#ifdef USE_NUMA
	if (exactListFound == false && biggerListFound == false)
	{
		// If we did not find any free chunk in specified NUMA, then lookup in other NUMA nodes
		// That makes sure, we do not fail our allocator if memory is exhausted in specified NUMA
		//map<int, NumaNode*>::iterator itNumaMap;
		//for (itNumaMap = mNumaNumberToNumaNodeMap.begin();
		//itNumaMap != mNumaNumberToNumaNodeMap.end() && itNumaMap->second != numa;
		//itNumaMap++)
		for (int i = 0; i < mNumaNumberToNumaNode.size() && i != node; i++)
		{
			//NumaNode* n = itNumaMap->second;
			NumaNode* n = mNumaNumberToNumaNode[i];
			SearchFreeListSmallestFirst(n, pSize, exactListFound, biggerListFound, iter);
			if (exactListFound || biggerListFound)
			{
				numa = n;
				break;
			}
		}
	}
#endif

  /* If we do not find any free chunk in all NUMAs including requested NUMA, we need more memory from
  // system. This situation should not arise really if we know in advance our usage limit. If our
  // heap would be too small, we waste time grabbing new chunk from the system. Hence select good
  // choice of heap size while initializing allocator (HeapInit())
	*/
	if (exactListFound == false && biggerListFound == false) // No space in any NUMA
	{

		int reqSize = HEAP_GROW_BY_SIZE;
		if (pSize > HEAP_GROW_BY_SIZE)
			reqSize = pSize;
		// add more heap in requested numa node
	  void* newChunk = SYS_MMAP_ALLOC(PageSizeToBytes(reqSize) );
	  FATALIF( !SYS_MMAP_CHECK(newChunk), "Run out of memory in Numa Allocator");

#ifndef STORE_HEADER_IN_CHUNK
		ChunkInfo* chunk = NULL;
		if (mDeletedChunks.empty())
			chunk = new ChunkInfo;
		else
		{
			chunk = mDeletedChunks.front();
			mDeletedChunks.pop_front();
		}
		chunk->currentPtr = newChunk;
		// add new chunk entry in map
		mPtrToChunkInfoMap[chunk->currentPtr] = chunk;
#else
		ChunkInfo* chunk = (ChunkInfo*)newChunk;
		chunk->currentPtr = (void*)((char*)newChunk+sizeof(ChunkInfo));
#endif
		chunk->prevChunk = NULL;
		chunk->nextChunk = NULL;
		chunk->numaNode = numa;
		(chunk->sizeInfo).sizeStruct.size = reqSize;
		(chunk->sizeInfo).sizeStruct.isFree = true;
		UpdateFreelist(chunk);
		// After adding above chunk, search again
		SearchFreeListSmallestFirst(numa, pSize, exactListFound, biggerListFound, iter);
	}

	/* If we found the bigger list, just carve out the chunk of requested size
	*/
	void * rezPtr; // the result pointer
	if (biggerListFound)
	{
		//ASSERT(r_iter != numa->mSizeToFreeListMap.rend());
		ASSERT(iter != numa->mSizeToFreeListMap.end());
		//ASSERT(!r_iter->second->empty());
		ASSERT(!iter->second->empty());
#ifndef STORE_HEADER_IN_CHUNK
		//rezPtr = *(r_iter->second->begin());
		rezPtr = *(iter->second->begin());
		map<void*, ChunkInfo*>::iterator it = mPtrToChunkInfoMap.find(rezPtr);
		ASSERT(it!=mPtrToChunkInfoMap.end());
		ChunkInfo* chunkInfo = it->second;
		ASSERT(rezPtr == chunkInfo->currentPtr);
#else
		//ChunkInfo* chunkInfo = (ChunkInfo*)((char*)(*(r_iter->second->begin())) - sizeof(ChunkInfo));
		ChunkInfo* chunkInfo = (ChunkInfo*)((char*)(*(iter->second->begin())) - sizeof(ChunkInfo));
		rezPtr = chunkInfo->currentPtr;
#endif
		ASSERT((chunkInfo->sizeInfo).sizeStruct.isFree);

#ifndef STORE_HEADER_IN_CHUNK
		ChunkInfo* chunk = NULL;
		if (mDeletedChunks.empty())
			chunk = new ChunkInfo;
		else
		{
			chunk = mDeletedChunks.front();
			mDeletedChunks.pop_front();
		}
		chunk->currentPtr = (void*)((char*)chunkInfo->currentPtr + PageSizeToBytes(pSize));
		// add new chunk entry in map
		mPtrToChunkInfoMap[chunk->currentPtr] = chunk;
#else
		ChunkInfo* chunk = (ChunkInfo*)((char*)chunkInfo->currentPtr + PageSizeToBytes(pSize));
		chunk->currentPtr = (void*)((char*)chunk + sizeof(ChunkInfo)) ;
#endif
		chunk->prevChunk = chunkInfo;
		chunk->nextChunk = chunkInfo->nextChunk;
		chunk->numaNode = numa;
		(chunk->sizeInfo).sizeStruct.size = (chunkInfo->sizeInfo).sizeStruct.size - pSize;
		(chunk->sizeInfo).sizeStruct.isFree = true;
		// remove from size freelist because it is occupied now
		RemoveFromFreelist(chunkInfo);

		if (chunkInfo->nextChunk)
			chunkInfo->nextChunk->prevChunk = chunk;
		chunkInfo->nextChunk = chunk;
		(chunkInfo->sizeInfo).sizeStruct.isFree = false;
		(chunkInfo->sizeInfo).sizeStruct.size = pSize;

		// add new chunk in size freelist
		UpdateFreelist(chunk);
	}
	/* If we found the exact size match, just return the chunk
	*/
	else if (exactListFound)
	{
		// we have the element
		set<void*>* setPtr = (*iter).second;
		rezPtr = *(setPtr->begin());
		ASSERT(!setPtr->empty());
		setPtr->erase(setPtr->begin()); // eliminate the front
		if (iter->second->empty())
		{
			mDeletedLists.push_back(setPtr);
			numa->mSizeToFreeListMap.erase(iter->first);
		}
		// Now mark chunk occupied
#ifndef STORE_HEADER_IN_CHUNK
		map<void*, ChunkInfo*>::iterator it = mPtrToChunkInfoMap.find(rezPtr);
		ASSERT(it!=mPtrToChunkInfoMap.end());
		ChunkInfo* chunkInfo = it->second;
		ASSERT(chunkInfo);
#else
		ChunkInfo* chunkInfo = (ChunkInfo*)((char*)rezPtr - sizeof(ChunkInfo));
#endif
		(chunkInfo->sizeInfo).sizeStruct.isFree = false;
	}
	/* This situation should never arise, if comes here, that would mean our logic is wrong
		 somewhere. Just assert
	*/
	else {
		FATALIF(true, "Page size not found = %d", pSize);
	}

	// test code
	// look up the new page to ensure it is not already out
	SizeMap::iterator it=sizeMap.find(rezPtr);
	FATALIF(it!=sizeMap.end(), "Allocating already allocated pointer %p.", rezPtr);
	// record the allocation in sizeMap
	sizeMap.insert(pair<void*, int>(rezPtr, pSize));

#ifdef MMAP_CHECK
	memChk.Insert(rezPtr, noBytes, f, l);
#endif
	pthread_mutex_unlock(&mutex);

	return rezPtr;
}


void NumaMemoryAllocator::MmapFree(void* ptr){
	if (ptr==NULL) {
		return;
    }

	pthread_mutex_lock(&mutex);

	set<void*>::iterator is = fixedSizeOccupiedList.find(ptr);
	if (is != fixedSizeOccupiedList.end()) {
		fixedSizeList.insert(ptr);
		fixedSizeOccupiedList.erase(is);
		pthread_mutex_unlock(&mutex);
		return;
	}
#ifdef MMAP_CHECK
	memChk.Delete(ptr);
#endif
	// see if the pointer is allocated
	FATALIF(sizeMap.size()==0, "I found the size map in MmapFree to be 0. This means that mmap_free was called before mmap_alloc");

	// find the size and insert the freed memory in the
	SizeMap::iterator it=sizeMap.find(ptr);
	FATALIF(it==sizeMap.end(), "Deallocating unallocated pointer %p.", ptr);
	// delete the element from the sizeMap.
	sizeMap.erase(it);

	Coalesce(ptr);

	pthread_mutex_unlock(&mutex);
}

// Update the freelist if some chunk is freed or allocated. If allocated, remove from freelist.
// If freed, add into the freelist under NUMA heap where it belongs. If given size freelist
// do not exist, create one and add into the map

void NumaMemoryAllocator::UpdateFreelist(ChunkInfo* chunkInfo){
	//chunkInfo->isFree = true;
	ASSERT((chunkInfo->sizeInfo).sizeStruct.isFree == true);
	map<int, set<void*>*>::iterator it = chunkInfo->numaNode->mSizeToFreeListMap.find((chunkInfo->sizeInfo).sizeStruct.size);
	// Not necessary that we find size freelist for chunk pointer to be added
	if (it != chunkInfo->numaNode->mSizeToFreeListMap.end())
		it->second->insert(chunkInfo->currentPtr);
	else
	{
		// we did not find the list; use old one or create it
		set<void*>* setPtr = NULL;
		if (mDeletedLists.empty())
			setPtr = new set<void*>;
		else
		{
			setPtr = mDeletedLists.front();
			mDeletedLists.pop_front();
		}
		// update the size map with new set
		chunkInfo->numaNode->mSizeToFreeListMap.insert(pair<int, set<void*>*>((chunkInfo->sizeInfo).sizeStruct.size, setPtr));
		// insert the chunk pointer in set
		setPtr->insert(chunkInfo->currentPtr);
	}
}

void NumaMemoryAllocator::RemoveFromFreelist(ChunkInfo* chunkInfo)
{
	// delete next chunk from size freelist
	ASSERT((chunkInfo->sizeInfo).sizeStruct.isFree == true);
	map<int, set<void*>*>::iterator it = chunkInfo->numaNode->mSizeToFreeListMap.find((chunkInfo->sizeInfo).sizeStruct.size);
	if (it != chunkInfo->numaNode->mSizeToFreeListMap.end())
	{
		ASSERT(!it->second->empty());
		it->second->erase(chunkInfo->currentPtr);
		if (it->second->empty())
		{
			mDeletedLists.push_back(it->second);
			chunkInfo->numaNode->mSizeToFreeListMap.erase((chunkInfo->sizeInfo).sizeStruct.size);
		}
	}
}

/* Merge the freed chunk with adjacent chunk (w.r.t physical memory). Below is the strategy,
// We are maintaining freelist as a link list where each chunk is pointing to previous chunk,
// next chunk can be obtained by using size information of current chunk. Hence effectively
// every chunk has previous and next link which needs to be updated once merging is done
// after merging, size increases, previous chunk pointer is updated for current node, and next
// to next node so that the integrity of link list is maintained. We need to make sure, that
// all nearby chunks have correct links and other header information. While merging, we also need
// to update freelist (remove smaller chunk info and add new bigger merged chunk info)
*/
void NumaMemoryAllocator::Coalesce(void* ptr)
{
	// find the chunk and insert the freed memory in the freelist
#ifndef STORE_HEADER_IN_CHUNK
	map<void*, ChunkInfo*>::iterator it = mPtrToChunkInfoMap.find(ptr);
	ASSERT(it!=mPtrToChunkInfoMap.end());
	ChunkInfo* chunkInfo = it->second;
#else
	ChunkInfo* chunkInfo = (ChunkInfo*)((char*)ptr-sizeof(ChunkInfo));
#endif
	ASSERT(chunkInfo);
	ASSERT((chunkInfo->sizeInfo).sizeStruct.isFree == false);

	//cout << "NUMA: << " << chunkInfo->currentPtr << "\t" << (chunkInfo->sizeInfo).sizeStruct.size << endl;
	bool isCoalesce = false;
	if ((chunkInfo->sizeInfo).sizeStruct.size > NO_COALESCE_MAXPAGESIZE)
		isCoalesce = true;

	if (isCoalesce == true) {
		bool isPrevChunkMerged = false;
		// Merge with previous chunk
		//if (chunkInfo->prevChunk && (chunkInfo->prevChunk->sizeInfo).sizeStruct.size > NO_COALESCE_MAXPAGESIZE && (chunkInfo->prevChunk->sizeInfo).sizeStruct.isFree) {
		if (chunkInfo->prevChunk && (chunkInfo->prevChunk->sizeInfo).sizeStruct.isFree) {
			isPrevChunkMerged = true;
			// remove from size freelist before size updates
			RemoveFromFreelist(chunkInfo->prevChunk);
			// Update previous chunk
			(chunkInfo->prevChunk->sizeInfo).sizeStruct.size = (chunkInfo->prevChunk->sizeInfo).sizeStruct.size + (chunkInfo->sizeInfo).sizeStruct.size;
			chunkInfo->prevChunk->nextChunk = chunkInfo->nextChunk;
			//Update next chunk
			if (chunkInfo->nextChunk)
				chunkInfo->nextChunk->prevChunk = chunkInfo->prevChunk;
		}
		// Merge with next chunk
		//if (chunkInfo->nextChunk && (chunkInfo->nextChunk->sizeInfo).sizeStruct.size > NO_COALESCE_MAXPAGESIZE && (chunkInfo->nextChunk->sizeInfo).sizeStruct.isFree) {
		if (chunkInfo->nextChunk && (chunkInfo->nextChunk->sizeInfo).sizeStruct.isFree) {
			ChunkInfo* next = chunkInfo->nextChunk;
			RemoveFromFreelist(next);
			if (chunkInfo->prevChunk && (chunkInfo->prevChunk->sizeInfo).sizeStruct.isFree) {
				// update previous chunk details
				(chunkInfo->prevChunk->sizeInfo).sizeStruct.size = (chunkInfo->prevChunk->sizeInfo).sizeStruct.size + (chunkInfo->nextChunk->sizeInfo).sizeStruct.size;
				chunkInfo->prevChunk->nextChunk = chunkInfo->nextChunk->nextChunk;
				// update next to next chunk
				if (chunkInfo->nextChunk->nextChunk)
				chunkInfo->nextChunk->nextChunk->prevChunk = chunkInfo->prevChunk;
			}
			else {
				// update next to next chunk
				if (chunkInfo->nextChunk->nextChunk)
					chunkInfo->nextChunk->nextChunk->prevChunk = chunkInfo;
				// update current chunk
				(chunkInfo->sizeInfo).sizeStruct.size = (chunkInfo->sizeInfo).sizeStruct.size + (chunkInfo->nextChunk->sizeInfo).sizeStruct.size;
				chunkInfo->nextChunk = chunkInfo->nextChunk->nextChunk;
			}
#ifndef STORE_HEADER_IN_CHUNK
			// erase from map
			map<void*, ChunkInfo*>::iterator itc = mPtrToChunkInfoMap.find(next->currentPtr);
			ASSERT(itc != mPtrToChunkInfoMap.end());
			mPtrToChunkInfoMap.erase(itc);
			//delete next chunk
			mDeletedChunks.push_back(next);
#endif
		}

		if (isPrevChunkMerged) {
			UpdateFreelist(chunkInfo->prevChunk);
#ifndef STORE_HEADER_IN_CHUNK
			//delete current chunk
			mDeletedChunks.push_back(chunkInfo);
			//remove from map
			mPtrToChunkInfoMap.erase(it);
#endif
		}
		else {
			(chunkInfo->sizeInfo).sizeStruct.isFree = true;
			UpdateFreelist(chunkInfo);
		}
	} else {
			(chunkInfo->sizeInfo).sizeStruct.isFree = true;
			UpdateFreelist(chunkInfo);
	}
}

NumaMemoryAllocator::~NumaMemoryAllocator(void){
	// dealocate the mutex
	pthread_mutex_destroy(&mutex);
	// it would be nice to deallocate the memory with munmap as well
}

size_t NumaMemoryAllocator::AllocatedPages() {
	size_t Size = 0;
	for (map<void*, ChunkInfo*>::const_iterator it = mPtrToChunkInfoMap.begin(); it != mPtrToChunkInfoMap.end(); ++it) {
		if (!((it->second)->sizeInfo).sizeStruct.isFree)
			Size += ((it->second)->sizeInfo).sizeStruct.size;
	}
	return Size;
}

size_t NumaMemoryAllocator::FreePages() {
	size_t totalFreelistSize = 0;
	for (int i = 0; i < mNumaNumberToNumaNode.size(); i++) {
		for (map<int, set<void*>*>::iterator it = (mNumaNumberToNumaNode[i]->mSizeToFreeListMap).begin();
				it != (mNumaNumberToNumaNode[i]->mSizeToFreeListMap).end(); it++) {
			totalFreelistSize += (it->first) * (it->second)->size();
		}
	}
	return totalFreelistSize;
}

#ifdef MMAP_CHECK
void NumaMemoryAllocator::Diagnose() {
	pthread_mutex_lock(&mutex);
	memChk.Print();
	printf("\n =================  %ld\n", AllocatedPages());
	pthread_mutex_unlock(&mutex);
}
#endif
