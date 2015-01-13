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

#ifndef COL_STOR_H
#define COL_STOR_H

#include "RawStorageDesc.h"
#include "DistributedCounter.h"

// This class encapsultes storage management for a column... it is a pure abstract class
// It is designed so that it you can have compressed storage, uncompressed storage, different
// kinds of compressed storage, etc., and move easily (and automatically) between the types
//
class ColumnStorage {

	#include "ColumnStoragePrivate.h"

public:

	// tells us if this is the lone copy.  This function is important because it 
	// assures the object that there are not any other shallow copies floating around 
	// out there.  The destructor of any class derived from the ColumnStorage class
	// should have the following code:
	//
	// if (IsLoneCopy ())
	// 	Finalize ();
	int IsLoneCopy ();

	// these functions must be provided by any actual ColumnStorage implementation
	
	// creates and returns a "shallow" copy of this object to the caller.  The "shallow"
	// part means that the actual data is simply aliased, but any state that might be
	// associated with the storage is unique to the copy
	virtual ColumnStorage *CreateShallowCopy () = 0;

	// all of the memory in this object that can be shared accross shallow copies is
	// deallocated by this function.  Any memory that is unique to the shallow copy
	// should be destroyed by the dectructor associated with the specific storage type
	virtual void Finalize () = 0;

	// returns a pointer to at least numBytesRequested bytes of data in the column.
	// The actual number of bytes returned is put into numBytesRequested.
	virtual char *GetData (int posToStartFrom, int &numBytesRequested) = 0;

	// tells the storage that we are done with this round of iteration, and that we
	// want the column to be truncated at numBytes in length.  Optionally, the storage
	// can return a pointer to a new storage object that should be used to replace itself.
	// This is useful, for example, so that the storage can decompress itself as it is
	// processed, and then return a decompressed version of itself for future use.
	virtual ColumnStorage *Done (int numBytes) = 0;

	// this tells the storage that we want it to make a deep copy of all of the data
	// structures that were simply aliased during a shallow copy.  The existing verions
	// of those data structures should simply be over-written, and not freed; the
	// ColumnStorage object iself will free those if needed
	//virtual void Detach () = 0;

	// this tells the storage that we want it to make a deep copy of all of the data
	// and return the new storage
	virtual ColumnStorage *CreatePartialDeepCopy (int position) = 0;

	// returns the number of valid bytes in the column
	virtual int GetNumBytes () = 0;

public:

	// constructor and destructor
	ColumnStorage ();
	virtual ~ColumnStorage ();

	// There is no swap operator, since a ColumnStorage object should be created and
	// then immediately loaded up into a Column object!
	
	// Any class that actually implements the ColumnStorage will add routines that 
	// put data into the storage, and perhaps routines that read/write the storage to disk
	
	// Note that the destrcutor of any class that actually implements the ColumnStorage
	// should always free memory asociated with components that are NOT shared among shallow
	// copies of the storage

	// API to compress and get access to the raw data. This is used by the IO subsystem
	// see Column.h for the explanation of how they behave
	virtual void Compress(bool deleteDecompressed)=0;

	// Get the handle of the compressed storage
	virtual void GetCompressed(RawStorageList& where)=0;

	// Get the compressed size in bytes
	virtual off_t GetCompressedSizeBytes() = 0;

	// Get the compressed size in pages
	virtual off_t GetCompressedSizePages() = 0;

	// Get the handle of the uncompressed storage
	virtual void GetUncompressed(RawStorageList& where)=0;

	// Get the uncompressed size in bytes
	virtual off_t GetUncompressedSizeBytes() = 0;

	// Get the uncompressed size in pages
	virtual off_t GetUncompressedSizePages() = 0;

	// If this storage is compressed, which is true if we receive compressed storage from outside
	virtual bool GetIsCompressed () = 0;

	// get if it is readonly or writeonly
	virtual bool IsWriteMode () = 0;

};

#endif
