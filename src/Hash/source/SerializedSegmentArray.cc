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

#include "SerializedSegmentArray.h"
#include "MmapAllocator.h"

SerializedSegmentArray :: SerializedSegmentArray () {
  arrayLenHashes = arrayLenSegs = MMAP_PAGE_SIZE/sizeof(HashEntry);
	lastUsedHash = lastUsedSeg = 0;	
	allHashes = (HT_INDEX_TYPE *) mmap_alloc (sizeof (HT_INDEX_TYPE) * arrayLenHashes, 1);
	myData = (HashEntry *) mmap_alloc (sizeof (HashEntry) * arrayLenSegs, 1);
}

SerializedSegmentArray :: ~SerializedSegmentArray () {
	mmap_free (allHashes);
	mmap_free (myData);
}

void SerializedSegmentArray :: DoubleHashArray () {
	HT_INDEX_TYPE *temp = (HT_INDEX_TYPE *) mmap_alloc (sizeof (HT_INDEX_TYPE) * arrayLenHashes * 2, 1);
	memmove (temp, allHashes, sizeof (HT_INDEX_TYPE) * arrayLenHashes);
	mmap_free (allHashes);
	allHashes = temp;
	arrayLenHashes *= 2;
}

void SerializedSegmentArray :: DoubleEntryArray () {
	HashEntry *temp = (HashEntry *) mmap_alloc (sizeof (HashEntry) * arrayLenSegs * 2, 1);
	memmove (temp, myData, sizeof (HashEntry) * arrayLenSegs);
	mmap_free (myData);
	myData = temp;
	arrayLenSegs *= 2;
}

