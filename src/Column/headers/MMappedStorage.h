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

#ifndef MMAP_STOR_H
#define MMAP_STOR_H

#include "ColumnStorage.h"
#include "StorageUnit.h"
#include "CompressibleStorageUnit.h"
#include "quicklz.h"
#include "RawStorageDesc.h"
#include "FileMetadata.h"
#include "DistributedCounter.h" // REMOVE

// this macro controls in-place decompression
//#define DECOMPRESS_IN_PLACE 1

class MMappedStorage : public ColumnStorage {

private:
	
	// the contigous storage of decompressed data is made of concatenation
	// of elements of storage
	TwoWayList <StorageUnit> storage;
	
	// total size in bytes
	int numBytes;
	// This is size after compressing data. This is not the size of compressed data
	// received from disk
	int numCompressedBytes;

	// the bridge is used to allow a "flat view" outside but broken inside
	StorageUnit bridge;
	bool bridgeEmpty; // is the bridge empty? This is used to avoid killing bridge

	// compressed data from the disk
	CompressedStorageUnit cstorage;

	bool decompress; // should we decompress?

	// read or write mode?
	bool isWriteMode;

	// numa node
	int numa;

public:

	// these are the standard pure virtual functions any MMappedStorage must provide
	MMappedStorage *CreateShallowCopy ();
	
	// Finalize is empty. We can dealocate in destructor
	void Finalize(){}

	char *GetData (int posToStartFrom, int &numBytesRequested);
	MMappedStorage *Done (int numBytes);
	//void Detach ();
	MMappedStorage *CreatePartialDeepCopy (int position);
	int GetNumBytes ();

public:

	static DistributedCounter* storeCount; // REMOVE
	static DistributedCounter* createCount; // REMOVE

	// Compress the storage
	void Compress(bool);

	// Get the handle of the compressed storage
	void GetCompressed(RawStorageList&);

	// Get the compressed size in bytes
	off_t GetCompressedSizeBytes();

	// Get the compressed size in pages
	off_t GetCompressedSizePages();

	// Get the handle of the uncompressed storage
	void GetUncompressed(RawStorageList&);

	// Get the uncompressed size in bytes
	off_t GetUncompressedSizeBytes();

	// Get the uncompressed size in pages
	off_t GetUncompressedSizePages();

	// If this storage is compressed, which is true if we receive compressed storage from outside
	bool GetIsCompressed ();

	// If this storage is write only or read only?
	bool IsWriteMode ();

	// This receives storage from outside, either compressed or uncompressed
	// Hence it is read only storage. Passing allocated space considering it blank
	// will not work
	MMappedStorage (void *myData, int numBytes, int numCompressedBytes, int numaNode = 0);

	// Special constructor to read a partial chunk
	// Arguments:
	//    myData: allocated data of size numPages; 
	//    preEmptyPages, postEmptyPages: size in pages of pre and post invalid regions
	//		frag: metadata details about the fragment (this chunk may be partial fragment)
	//
	// LAYOUT:
	// ------------------------------------------------------------------
	// | preEempty (NULL) | myData (numPages mapped) | postEmpty (NULL) |
	// ------------------------------------------------------------------
	MMappedStorage (void *myData, int preEmptyPages, int numPages, 
									int postEmptyPages, int numaNode = 0);

	// This is blank storage, used for writing from scratch
	// Hence it is write only mode
	MMappedStorage (int numaNode = 0);

	void swap(MMappedStorage& withMe){
	  SWAP_memmove(MMappedStorage, withMe)
	}

	// destruct
	~MMappedStorage ();
};

inline
bool MMappedStorage :: IsWriteMode () {return isWriteMode;}

#endif
