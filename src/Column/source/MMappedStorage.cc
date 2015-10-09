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

#include "MMappedStorage.h"
#include "TwoWayList.cc"
#include "MmapAllocator.h"
#include "RawStorageDesc.h"
#include <assert.h> // sarvesh remove
#include<map>
#include<stdio.h>

using namespace std;
#include <iostream>

// forward decls of the allocation functions

MMappedStorage *MMappedStorage :: CreateShallowCopy () {

	// create the guy we are gonna return
	MMappedStorage *returnVal = new MMappedStorage;
	returnVal->numBytes = numBytes;

	// and copy over all of the RAM pages
	storage.MoveToStart ();
	while (storage.RightLength ()) {
		StorageUnit temp;
		temp.copy (storage.Current ());
		returnVal->storage.Insert (temp);
		storage.Advance ();
		returnVal->storage.Advance ();
	}

	// copy compressed storage too
	returnVal->cstorage.copy (cstorage);
	returnVal->decompress = decompress;
	returnVal->isWriteMode = isWriteMode;
	returnVal->numa = numa;

	return returnVal;
};

MMappedStorage :: MMappedStorage (uint64_t numaNode) :
    ColumnStorage(),
    storage(),
    numBytes(0),
    numCompressedBytes(0),
    bridge(),
	// remember that we are empty
    bridgeEmpty(true),
    bridgeSize(0),
    allocMultiplier(1),
    cstorage(),
    decompress(false),
	// this can be treated as write only storage, because we dont have anything to read
    isWriteMode(true),
    numa(numaNode)
{
}


	/******** THIS IS AN EXPERIMENTAL VERSION THAT HAS A PROBLEM 
char *MMappedStorage :: GetData (uint64_t posToStartFrom, uint64_t &numBytesRequested) {
	// super fast path: the request is within the current storage element and it fits
	// completely in
	// 
	if (storage.Current ().start <= posToStartFrom && storage.Current ().end > posToStartFrom+numBytesRequested)
		return storage.Current ().bytes + (posToStartFrom - storage.Current ().start);

	// This is special case handled for 80 % of cases where we are reading from disk and size of storage unit is
	// one. It is fast path. It helps to store pointer while doing CheckpointStore in JoinMerge

	if (storage.Length () == 1 && !isWriteMode && storage.Current ().start <= posToStartFrom && storage.Current ().end > posToStartFrom) {
		if (posToStartFrom + numBytesRequested - 1 <= storage.Current ().end) {
			numBytesRequested = storage.Current ().end - posToStartFrom;
		}
		return storage.Current ().bytes + (posToStartFrom - storage.Current ().start);
	}

	// first, if we have a "bridge" and we have written to it, then copy the written bytes
	// the bridge can be written at any time so we can delay this
	if (!bridgeEmpty && isWriteMode) {
		storage.MoveToStart ();
		while (storage.RightLength ()) {
			storage.Current ().CopyOverlappingContent (bridge);
			storage.Advance ();
		}
	}
	bridgeEmpty = true;

	// now, go through the list again and see if any memory block covers the request
	storage.MoveToStart ();
	while (storage.RightLength ()) {
		// see if the current entry covers the requet
		if (storage.Current ().start <= posToStartFrom &&
		    storage.Current ().end >= posToStartFrom) {
			// it does, so see if it totally covers it
			if (posToStartFrom + numBytesRequested - 1 <= storage.Current ().end) {
				// tell the caller the actual number of bytes he can use
				numBytesRequested = storage.Current ().end - posToStartFrom + 1;
				return storage.Current ().bytes + (posToStartFrom - storage.Current ().start);
			// the page does not totally cover the request, so create a bridge
			// that is a contiguous block of storage covering the entire request
			} else {
			  FATALIF(numBytesRequested>MMAP_PAGE_SIZE, "The request is too large");
			  if (bridge.bytes == NULL) // need a bridge so make sure we have one
			    bridge.bytes = (char *) mmap_alloc (MMAP_PAGE_SIZE, numa); // larage bridge.

			  bridge.start = posToStartFrom;
			  bridge.end = posToStartFrom + numBytesRequested - 1;
			  bridgeEmpty = false;

			  if (isWriteMode){
			  	StorageUnit temp;
			  	temp.start = storage.Current().end+1; // start just after end of the previous one
			  	temp.end = temp.start + MMAP_PAGE_SIZE - 1; // end one page latter (inclusive)

				// alloc the new chunk and then insert it!
			  	temp.bytes = (char *)mmap_alloc(MMAP_PAGE_SIZE, numa);
			  	storage.Append(temp);
			  } else { // reading mode
				while (storage.RightLength ()) {
					bridge.CopyOverlappingContent (storage.Current ());
					storage.Advance ();
				}
			  }
			  // and return the bridge!
			  return bridge.bytes;
			}
		}
		storage.Advance ();
	}

	// if we got here, then no existing chunk of memory covers the request; so, create it
	// This can happen if the r'equests are alligned and we never need a bridge
	StorageUnit temp;
	temp.start = posToStartFrom;
  	temp.end = temp.start + MMAP_PAGE_SIZE - 1; // end one page latter (inclusive)
	temp.bytes = (char *) mmap_alloc (MMAP_PAGE_SIZE, numa);

	// insert the new chunk of memory
	char *returnVal = temp.bytes;
	storage.Insert (temp);

	// and get outta here!
	return returnVal;
}
******************/

char *MMappedStorage :: GetData (uint64_t posToStartFrom, uint64_t &numBytesRequested) {

	/*-------------------- Below new added code for compression ---------------*/

	// below code decompresses data and assign a one big chunk of
	// storage unit to the decompressed data.
	uint64_t decompressedbytes = 0;
	if (decompress) {
		// We can not decompress in place for writing case, since we need whole data for writing on disk
		// so just decompress in place for read only case
		if (!isWriteMode)
//#ifdef DECOMPRESS_IN_PLACE
			decompressedbytes = cstorage.DecompressInPlace(posToStartFrom, posToStartFrom+numBytesRequested);
//#else
		else
			decompressedbytes = cstorage.DecompressUpTo(posToStartFrom+numBytesRequested);
//#endif
	}
	/*-------------------- Above new added code for compression ---------------*/

	// This is special case handled for 80 % of cases where we are reading from disk and size of storage unit is
	// one. It is fast path. It helps to store pointer while doing CheckpointStore in JoinMerge

	if (storage.Length () == 1 && !isWriteMode && storage.Current ().start <= posToStartFrom && storage.Current ().end > posToStartFrom) {
		if (posToStartFrom + numBytesRequested - 1 > storage.Current ().end) {
			numBytesRequested = storage.Current ().end - posToStartFrom;
		}
		return storage.Current ().bytes + (posToStartFrom - storage.Current ().start);
	}


	// first, if we have a "bridge" and we have written to it, then copy the written bytes
	if (!bridgeEmpty && isWriteMode) {
		storage.MoveToStart ();
		while (storage.RightLength ()) {
			storage.Current ().CopyOverlappingContent (bridge);
			storage.Advance ();
		}
	}
	bridgeEmpty = true;

	// now, go through the list again and see if any memory block covers the request
	storage.MoveToStart ();
	while (storage.RightLength ()) {

		// see if the current entry covers the requet
		if (storage.Current ().start <= posToStartFrom &&
		    storage.Current ().end >= posToStartFrom) {

			// it does, so see if it totally covers it
			if (posToStartFrom + numBytesRequested - 1 <= storage.Current ().end) {

				// tell the caller the actual number of bytes he can use
	/*-------------------- Below new added code for compression ---------------*/
				if (decompress && (decompressedbytes < storage.Current ().end))
					numBytesRequested = decompressedbytes - posToStartFrom;
				else
	/*-------------------- Above new added code for compression ---------------*/
					numBytesRequested = storage.Current ().end - posToStartFrom + 1;

				// and return them!
//#ifdef DECOMPRESS_IN_PLACE
			if (!isWriteMode) {
				if (decompress)
					return storage.Current ().bytes;
				else
					return storage.Current ().bytes + (posToStartFrom - storage.Current ().start);
			}
			else {
//#else
				return storage.Current ().bytes + (posToStartFrom - storage.Current ().start);
			}
//#endif


			// the page does not totally cover the request, so create a bridge
			// that is a contiguous block of storage covering the entire request
			} else {
        if (numBytesRequested > bridgeSize) {
          // Deallocate previous bridge.
          if (bridge.bytes != nullptr){
	    // free the old bridge
	    mmap_free(bridge.bytes);
	    // change the allocator size so that we can write MIN_DATA_IN_ALLOC_UNIT objects
	    allocMultiplier = BYTES_TO_PAGES(numBytesRequested*MIN_DATA_IN_ALLOC_UNIT);
	  }
          // Allocate a bigger bridge to accomodate large request.
          bridgeSize = PAGE_ALIGN(numBytesRequested);
          bridge.bytes = (char *) mmap_alloc (bridgeSize, numa); // larage bridge.
        }
        FATALIF(numBytesRequested > bridgeSize, "bridge is too small");
			  bridge.start = posToStartFrom;
			  bridge.end = posToStartFrom + numBytesRequested - 1;
			  bridgeEmpty = false;

				uint64_t upperEnd;
				while (storage.RightLength ()) {
					bridge.CopyOverlappingContent (storage.Current ());
					upperEnd = storage.Current ().end;
					storage.Advance ();
				}

				// if no existing chunk of memory covers the last part of the bridge,
				// then we need to create it
				if (upperEnd < bridge.end) {
					StorageUnit temp;
					temp.start = upperEnd + 1;
					temp.end = temp.start + allocMultiplier* MMAP_PAGE_SIZE - 1;

					// make sure we cover the request
					if (temp.end < bridge.end)
						temp.end = bridge.end;

					// alloc the new chunk and then insert it!
					temp.bytes = (char *)mmap_alloc
						(temp.end - temp.start + 1, numa);
					storage.Insert (temp);

				}

				// and return the bridge!
				return bridge.bytes;
			}

		}

		storage.Advance ();

	}

	// if we got here, then no existing chunk of memory covers the request; so, create it
	StorageUnit temp;
	temp.start = posToStartFrom;
	numBytesRequested = PAGES_TO_BYTES(BYTES_TO_PAGES(numBytesRequested));
	if (numBytesRequested < MMAP_PAGE_SIZE)
	  numBytesRequested = MMAP_PAGE_SIZE;
	temp.end = posToStartFrom + numBytesRequested - 1;
	temp.bytes = (char *) mmap_alloc (numBytesRequested, numa);

	// insert the new chunk of memory
	char *returnVal = temp.bytes;
	storage.Insert (temp);

	// and get outta here!
	return returnVal;
}



void MMappedStorage :: MakeReadonly(){
	storage.MoveToStart ();
	while (storage.RightLength ()) {
		storage.Current ().MakeReadonly();
		storage.Advance ();
	}
}

MMappedStorage *MMappedStorage :: Done (uint64_t num) {

	// first, if we have a "bridge" and we have written to it, then copy the written bytes
	if (!bridgeEmpty && isWriteMode) {
		storage.MoveToStart ();
		while (storage.RightLength ()) {
			storage.Current ().CopyOverlappingContent (bridge);
			storage.Advance ();
		}
	}
	bridgeEmpty = true;

	// make all the storage units Readonly since we do not allow the chunk to be changed 
	// beyound this point
	MakeReadonly();

	// truncate the column
	numBytes = num;

	// change the mode to read only
	isWriteMode = false;

	// note that we do not return any special version of ourself
	return 0;
}

#if 0 // not needed for now
void MMappedStorage :: Detach () {

  // we just need to make a deep copy of every storage item
  TwoWayList <StorageUnit> newStorage;
  storage.MoveToStart ();
  while (storage.RightLength ()) {
    StorageUnit old, newone;
    storage.Remove (old);
    newone.copy (old);
    newone.bytes = (char *) mmap_alloc (newone.end - newone.start + 1);
    memmove (newone.bytes, old.bytes, newone.end - newone.start + 1);
    newStorage.Insert (newone);
    newStorage.Advance ();
  }

  // and swap in the new storage
  storage.swap (newStorage);

  // create a new store for compressed unit
  CompressedStorageUnit newcs;
  //cstorage.Detach(newcs);
  cstorage.CreateDeepCopy(newcs);
  cstorage.swap(newcs);
}
#endif

MMappedStorage *MMappedStorage :: CreatePartialDeepCopy (uint64_t position) {

	// create the guy we are gonna return
	// create new guy into same numa node until we enhance it not to
	MMappedStorage *returnVal = new MMappedStorage(numa);
	returnVal->numBytes = numBytes;

	// we just need to make a deep copy of every storage item
	TwoWayList <StorageUnit> newStorage;
	storage.MoveToStart ();
	while (storage.RightLength () && storage.Current ().start <= position) {
		StorageUnit newone(storage.Current(), numa);
		newStorage.Insert (newone);
		newStorage.Advance ();
		storage.Advance ();
	}

	// and swap in the new storage
	returnVal->storage.swap (newStorage);

	// copy compressed storage too
	CompressedStorageUnit newcs;
	returnVal->cstorage.CreateDeepCopy (newcs);
	returnVal->cstorage.swap(newcs);
	returnVal->decompress = decompress;

	// as soon as we deep copy, our mode changes to writeonly
	// because that is when we will want deep copy
	returnVal->isWriteMode = true;

	return returnVal;
}

uint64_t MMappedStorage :: GetNumBytes () {
	return numBytes;
}

void MMappedStorage :: Compress(bool deleteDecompressed) {
	// create a compressed storage unit and store it into cStorage
	// compress into same numa until enhanced not to
	CompressedStorageUnit cUnit(GetNumBytes(), numa);

	// it is crucial to check that the storage units are in order
	int prevPos = -1;

	// walk through the storage units and ask compressor to compress
	storage.MoveToStart ();
	while (storage.RightLength ()) {
		StorageUnit& unit = storage.Current();
		FATALIF(unit.start -1 != prevPos, "Storage units not in order");
		// No need to compress extra unfilled bytes, hence correct the size if it increases numBytes
		//if (unit.end > numBytes) // sarvesh, commented these, becoz they modify storage !!
		//	unit.end = numBytes;
		prevPos = unit.end;
		cUnit.CompressThisStorageUnit(unit);
		storage.Advance();
	}

	cstorage.swap(cUnit);

	if (deleteDecompressed) {
	  // simply remove all of the data and free it
 	 	storage.MoveToStart ();
 	 	while (storage.RightLength ()) {
 	 	  StorageUnit temp;
 	 	  storage.Remove (temp);
 	 	  temp.Kill ();
 	 	}
	}
}


bool MMappedStorage :: GetIsCompressed() {
	return cstorage.GetIsCompressed();
}

off_t MMappedStorage :: GetCompressedSizeBytes () {
	// return the actual compressed bytes stored at the time of compression
	return cstorage.GetCompressedSize();
}

off_t MMappedStorage :: GetCompressedSizePages () {
	// return the actual compressed bytes stored at the time of compression
	return BYTES_TO_PAGES(GetCompressedSizeBytes());
}

void MMappedStorage :: GetCompressed(RawStorageList& rawCList) {
	RawStorageList empty;
	rawCList.swap(empty); // clean up the content of the output

	cstorage.AddCompressedRawStorage(rawCList);
}

off_t MMappedStorage :: GetUncompressedSizeBytes () {
	return numBytes;
}

off_t MMappedStorage :: GetUncompressedSizePages () {
	return BYTES_TO_PAGES(numBytes);
}

void MMappedStorage :: GetUncompressed(RawStorageList& rawUncompressedList) {
	RawStorageList empty;
	rawUncompressedList.swap(empty); // clean up the content of the output

	storage.MoveToStart ();
	while (storage.RightLength ()) {
		StorageUnit& temp = storage.Current();
		// It may be possible that allocated chunk is bigger than actual data in it, possible
		// for last chunk in the list. In that case, correct the length
		uint64_t len = 0;
		if (temp.end > numBytes)
			len = numBytes - temp.start;
		else
			len = temp.end - temp.start + 1;
		RawStorageUnit sunit(temp.bytes, BYTES_TO_PAGES(len), len);
		rawUncompressedList.Append (sunit);
		storage.Advance();
	}
}

MMappedStorage :: MMappedStorage (void *myData, uint64_t sizeDecompressed, uint64_t sizeCompressed, uint64_t numaNode) :
    ColumnStorage(),
    storage(),
    numBytes(sizeDecompressed),
    numCompressedBytes(0),
    bridge(),
    bridgeEmpty(true),
    bridgeSize(0),
    allocMultiplier(1),
    cstorage(),
    decompress(false),
    isWriteMode(false),
    numa(numaNode)
{

	// It's always read mode, even if you pass the blank allocated storage
	// It is enforced to create empty MMapped storage constructor and it ensures
	// creation of storage with appropriate page sizes

	if (sizeCompressed == 0) { /* not compressed */
		// create a storage unit to store the data we have been passed
		StorageUnit temp(((char *) myData), 0, (sizeDecompressed - 1)); // compressed or not we get a single storage unit
		// and remember it
		storage.Insert (temp);
		decompress = false;
	} else {
		// create a compressed storage unit
		StorageUnit temp;
		CompressedStorageUnit cUnit((char*)myData, sizeDecompressed, sizeCompressed, temp, numa); // compressed or not we get a single storage unit
		cstorage.swap(cUnit);
		decompress = true;
		// and remember it
		storage.Insert (temp);
	}
}

MMappedStorage :: ~MMappedStorage () {
  // everything is destroied automagically
}

MMappedStorage :: MMappedStorage (void *myData, uint64_t preEmptyPages, uint64_t numPages, uint64_t postEmptyPages, uint64_t numaNode) :
    ColumnStorage(),
    storage(),
    numBytes((preEmptyPages + postEmptyPages + numPages) * MMAP_PAGE_SIZE),
    numCompressedBytes(0),
    bridge(),
    bridgeEmpty(true),
    bridgeSize(0),
    allocMultiplier(1),
    cstorage(),
    decompress(false),
    isWriteMode(false),
    numa(numaNode)
{
	// Pre empty pages in storage
	StorageUnit temp1(NULL, 0, preEmptyPages * MMAP_PAGE_SIZE);
	storage.Insert (temp1);

	// Actual pages
	StorageUnit temp2(((char *) ((char*)myData + preEmptyPages * MMAP_PAGE_SIZE)), 0, ((preEmptyPages + postEmptyPages) * MMAP_PAGE_SIZE - 1));
	storage.Insert (temp2);

	StorageUnit temp3(NULL, 0, numBytes - 1);
	storage.Insert (temp3);
}
