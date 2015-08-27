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

#include "Column.h"
#include "ColumnStorage.h"
#include <string.h>
#include "MMappedStorage.h"

uint64_t Column :: GetColLength () {
    return myData->GetNumBytes ();
}

Column :: ~Column () {

    // don't kill an empty column
    if (refCount == NULL)
        return;

    // if it is non-empty, then decrement the reference count
    if (refCount->Decrement(1) == 0){
        if( columnDestroyer )
            columnDestroyer(*this);

        delete refCount;
    }

    // and kill the storage
    delete myData;

}

void Column :: MakeReadonly(){ if (myData != NULL) { myData->MakeReadonly();} }

bool Column :: IsWriteMode () {
    return myData->IsWriteMode ();
}

char *Column :: GetNewData (uint64_t posToStartFrom, uint64_t &numBytesRequested) {
    FATALIF( myData == NULL, "Why is this NULL?");

    // this just goes right through to the storage
    return myData->GetData (posToStartFrom, numBytesRequested);
}

void Column :: Compress (bool deleteDecompressed) {
    myData->Compress (deleteDecompressed);
}

void Column :: GetCompressed(RawStorageList& where) {
    return myData->GetCompressed (where);
}

off_t Column :: GetCompressedSizeBytes() {
    return myData->GetCompressedSizeBytes();
}

off_t Column :: GetCompressedSizePages() {
    return myData->GetCompressedSizePages();
}

void Column :: GetUncompressed(RawStorageList& where) {
    return myData->GetUncompressed (where);
}

off_t Column :: GetUncompressedSizeBytes() {
    return myData->GetUncompressedSizeBytes();
}

off_t Column :: GetUncompressedSizePages() {
    return myData->GetUncompressedSizePages();
}

bool Column :: GetIsCompressed() {
    return myData->GetIsCompressed();
}

/*
   void Column :: Detach () {

// if we are invalid, just return
if (myMutex == 0)
return;

// first, make a deep copy if we need to
if (!myData->IsLoneCopy ()) {
myData->SetLoneCopy ();
myData->Detach ();
}

// now, we have to unlink ourselves from all of the copies of this data
pthread_mutex_lock (myMutex);

// if there are other copies, then we have to re-make our own book-keeping info
// and unlink ourself from those copies
if (*numCopies > 1) {
dataWeShouldBeUsing = new (MMappedStorage *);
 *dataWeShouldBeUsing = 0;
 (*numCopies)--;
 pthread_mutex_unlock (myMutex);
 myMutex = new pthread_mutex_t;
 pthread_mutex_init (myMutex, NULL);
 numCopies = new uint64_t;
 *numCopies = 1;

// otherwise, we are the only copy of the column storage
} else {
pthread_mutex_unlock (myMutex);
}
}
*/

void Column :: swap (Column &withMe) {
    SWAP_STD(refCount, withMe.refCount);
    SWAP_STD(myData, withMe.myData);
    fragments.swap(withMe.fragments);
    // SWAP_memmove is unsafe since fragments uses STL
}

Column :: Column (MMappedStorage &loadMe, Destroyer _dest ):
    columnDestroyer(_dest)
{

    // make a copy of the guy who is being sent in
    myData = new MMappedStorage;
    myData->swap(loadMe);

    // allocate everything
    refCount = new DistributedCounter(1);
}

Column :: Column () {
    refCount = NULL;  // marker for empty column
}

void Column :: copy (Column &copyMe) {

    // empty out this particular column
    Column empty;
    empty.swap (*this);

    if (copyMe.refCount == NULL)
        return; // no point in copying empty columns

    refCount = copyMe.refCount;
    fragments = copyMe.fragments;
    refCount->Increment(1);// one more copy

    // and now make a shallow copy of the storage
    myData = copyMe.myData->CreateShallowCopy ();

    // and remember that the new one is a copy of the existing one
    myData->SetCopyOf (*(copyMe.myData));
}

void Column :: Done (uint64_t numBytes) {

    if (refCount == 0)
        return;

    // see if the storage wants us to create a new version of itself
    MMappedStorage *newStorage = myData->Done (numBytes);

    FATALIF(newStorage != 0, "ColumnStorage that morphs into different storage not supported");
}

void Column :: CreatePartialDeepCopy (Column& fromMe, uint64_t curPosInColumn) {

    // If this call is coming from some valid column, dont do anything
    if (IsValid ())
        return;

    // make a deep copy of the guy who is being sent in
    myData = (fromMe.myData)->CreatePartialDeepCopy (curPosInColumn);

    // allocate everything
    refCount = new DistributedCounter(1);
}

Fragments& Column :: GetFragments () {

    /*  pthread_mutex_lock (myMutex);
        printf("\n  ------------------- total column count so far = %d, storeage count = %d, unit count = %ld", (Column::colCount)->GetCounter(), (MMappedStorage::storeCount)->GetCounter(), (StorageUnit::storeunitCount)->GetCounter()); fflush(stdout);
        pthread_mutex_unlock (myMutex);
        */
    return fragments;
}

void Column :: SetFragments (Fragments& _frag) {
    fragments = _frag;
}
