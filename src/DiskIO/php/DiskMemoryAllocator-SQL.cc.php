<?php
//
//  Copyright 2013 Tera Insights LLC
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
?>
<?php
require_once('SQLite.php');
?>




#include <sys/mman.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Errors.h"
#include "DiskMemoryAllocator.h"

using namespace std;

DiskMemoryAllocator::DiskMemoryAllocator()
{
    // initialize the mutex
    pthread_mutex_init(&mutex, NULL);
    mLastPage = 0;
}

// Helper function, to create new chunk or use already freed chunk
void DiskMemoryAllocator::CreateNewChunk(ChunkList* l) {
    ChunkInfo* chunk = NULL;
    if (mFreeChunks.empty()) // If chunk is not in freelist
    {
        chunk = new ChunkInfo;
        chunk->startPage = mLastPage;
        chunk->size = DISK_ALLOC_BLOCK_SIZE; // set the fix size of chunk
        chunk->nextFreePage = mLastPage;
        (l->listOfChunks).push_back(chunk); // insert into list of chunks
        mLastPage += DISK_ALLOC_BLOCK_SIZE; // update the global last free page
    }
    else	// If free chunk is found in freelist
    {
        chunk = mFreeChunks.front();
        mFreeChunks.pop_front();
        chunk->nextFreePage = chunk->startPage;
        (l->listOfChunks).push_back(chunk);
    }
}

off_t DiskMemoryAllocator::DiskAlloc(off_t pSize, uint64_t relID){

    //assert (pSize != 0);
    if (pSize == 0)
        return 0;

    pthread_mutex_lock(&mutex);
    off_t result = -1;

    map<uint64_t, ChunkList*>::iterator it = mRelationIDToChunkList.find(relID);
    if (it == mRelationIDToChunkList.end()) {
        /*If chunk list is not found w.r.t relation ID, we need to create first
            chunk and add it to the list. Use free chunks if already present in the
            free list.
        */
        ChunkList* l = new ChunkList;
        CreateNewChunk(l);
        mRelationIDToChunkList[relID] = l;
        ChunkInfo* c = (l->listOfChunks).back();
        result = c->nextFreePage; // Set the result page number
        c->nextFreePage += pSize;	// Update the next free page number

    } else {
        /*If chunk list is found w.r.t relation ID, we need to see if it fully
            satisfies the request. If not, create a new chunk and add into the list.
        */
        ChunkList* l = it->second;
        ChunkInfo* c = (l->listOfChunks).back();
        if (c->nextFreePage - c->startPage + pSize < c->size) {
            result = c->nextFreePage;   // Set the result page number
            c->nextFreePage += pSize;   // Update the next free page number
            FATALIF( c->nextFreePage > c->startPage + c->size, "Page alocator spilled over the disk page chunk");
        } else {
            CreateNewChunk(l);
            c = (l->listOfChunks).back();
            result = c->nextFreePage;   // Set the result page number
            c->nextFreePage += pSize;   // Update the next free page number
        }
    }

    pthread_mutex_unlock(&mutex);

    return result;
}


void DiskMemoryAllocator::DiskFree(uint64_t relID){

    pthread_mutex_lock(&mutex);

    map<uint64_t, ChunkList*>::iterator it = mRelationIDToChunkList.find(relID);
    assert (it != mRelationIDToChunkList.end());

    ChunkList* l = it->second;
    for (uint64_t i = 0; i < (l->listOfChunks).size(); i++) {
        mFreeChunks.push_back((l->listOfChunks)[i]);
    }
    delete it->second;
    mRelationIDToChunkList.erase(relID);
    pthread_mutex_unlock(&mutex);
}

DiskMemoryAllocator::~DiskMemoryAllocator(void){
    // dealocate the mutex
    pthread_mutex_destroy(&mutex);
    // it would be nice to deallocate the memory with munmap as well
}

off_t DiskMemoryAllocator::AllocatedSpace() {
    off_t allocated = 0;
    for (map<uint64_t, ChunkList*>::iterator it = mRelationIDToChunkList.begin(); it != mRelationIDToChunkList.end(); ++it) {
        ChunkList* l = it->second;
        for (uint64_t i = 0; i < (l->listOfChunks).size(); i++) {
            allocated += ((l->listOfChunks)[i]->nextFreePage - (l->listOfChunks)[i]->startPage);
        }
    }
    return allocated;
}

// This adds up all fragmented space including last chunk, later we can remove counting last chunk
// because essentially last chunk isn't fragmented yet, since it has opportunity to fulfill more
// requests.
off_t DiskMemoryAllocator::FragmentedSpace() {
    off_t fragmented = 0;
    for (map<uint64_t, ChunkList*>::iterator it = mRelationIDToChunkList.begin(); it != mRelationIDToChunkList.end(); ++it) {
        ChunkList* l = it->second;
        for (uint64_t i = 0; i < (l->listOfChunks).size(); i++) { // iterate one less if last chunk not to be counted
            fragmented += (l->listOfChunks)[i]->startPage + ((l->listOfChunks)[i]->size - (l->listOfChunks)[i]->nextFreePage);
        }
    }
    return fragmented;
}

void DiskMemoryAllocator::Flush(sqlite3* db) {

    pthread_mutex_lock(&mutex);
    PDEBUG("DiskMemoryAllocator::Flush()");
<?php
grokit\sql_existing_database( 'db' );
?>
;

<?php
grokit\sql_statements_norez( <<<'EOT'
"
    /* Relations */
    CREATE TABLE IF NOT EXISTS DiskAllocatorState (
      arrayID        INTEGER,
      relID          INTEGER,
      startPage          INTEGER,
      size     INTEGER,
      nextFreePage    INTEGER,
      lastPage        INTEGER
    );
  "
EOT
, [ ] );
?>
;

  // delete all content
<?php
grokit\sql_statements_norez( <<<'EOT'
"
    DELETE FROM DiskAllocatorState
    WHERE arrayID=%d;
  "
EOT
, [ 'mDiskArrayID', ] );
?>
;

<?php
grokit\sql_statement_parametric_norez( <<<'EOT'
"
    INSERT INTO DiskAllocatorState(arrayID, relID, startPage, size, nextFreePage, lastPage)
    VALUES (?1, ?2, ?3, ?4, ?5, ?6);
  "
EOT
, [ 'int', 'int', 'int', 'int', 'int', 'int', ], [ ]);
?>
;
    // iterate allocated chunks
    for (map<uint64_t, ChunkList*>::iterator it = mRelationIDToChunkList.begin(); it != mRelationIDToChunkList.end(); ++it) {
      uint64_t rel = it->first;
            ChunkList* l = it->second;
            for (uint64_t i = 0; i < (l->listOfChunks).size(); i++) {
                uint64_t startPg = ((l->listOfChunks)[i]->startPage);
                uint64_t sz = ((l->listOfChunks)[i]->size);
                uint64_t n = (l->listOfChunks)[i]->nextFreePage;
                //printf("startPg = %d, sz = %d, n = %d, rel = %d, diskArrayID = %d, lastPage = %d\n", startPg, sz, n, rel, mDiskArrayID, mLastPage);
<?php
grokit\sql_instantiate_parameters( [ 'mDiskArrayID', 'rel', 'startPg', 'sz', 'n', 'mLastPage', ] );
?>
;
            }
    }
        for (list<ChunkInfo*>::iterator iter = mFreeChunks.begin(); iter != mFreeChunks.end(); ++iter) {
            uint64_t rel = -1;
            uint64_t startPg = (*iter)->startPage;
            uint64_t sz = (*iter)->size;
            uint64_t n = (*iter)->nextFreePage;
            //printf("-1 startPg = %d, sz = %d, n = %d, rel = %d, diskArrayID = %d, lastPage = %d\n", startPg, sz, n, rel, mDiskArrayID, mLastPage);
<?php
grokit\sql_instantiate_parameters( [ 'mDiskArrayID', 'rel', 'startPg', 'sz', 'n', 'mLastPage', ] );
?>
;
        }
<?php
grokit\sql_parametric_end();
?>
;
    pthread_mutex_unlock(&mutex);
}

void DiskMemoryAllocator::Load(sqlite3* db) {
    pthread_mutex_lock(&mutex);
<?php
grokit\sql_existing_database( 'db' );
?>
;

<?php
grokit\sql_statements_norez( <<<'EOT'
"
        CREATE TABLE IF NOT EXISTS DiskAllocatorState (
            arrayID        INTEGER,
          relID          INTEGER,
          startPage      INTEGER,
            size           INTEGER,
          nextFreePage   INTEGER,
          lastPage       INTEGER
        );
  "
EOT
, [ ] );
?>
;

<?php
grokit\sql_statement_table( <<<'EOT'
"
      SELECT relID, startPage, size, nextFreePage, lastPage
      FROM DiskAllocatorState
      WHERE arrayID=%d;
    "
EOT
, [ 'relID' => 'int', 'startPage' => 'int', 'size' => 'int', 'nextFreePage' => 'int', 'LastPage' => 'int', ], [ 'mDiskArrayID', ] );
?>
{
        ChunkInfo* c = new ChunkInfo;
        c->startPage = startPage;
        c->size = size;
        c->nextFreePage = nextFreePage;
        mLastPage = LastPage;
        if (relID != -1) {
            map<uint64_t, ChunkList*>::iterator it = mRelationIDToChunkList.find(relID);
            if (it == mRelationIDToChunkList.end()) {
                ChunkList* l = new ChunkList;
                (l->listOfChunks).push_back(c);
                mRelationIDToChunkList[relID] = l;
            } else {
                ChunkList* l = it->second;
                (l->listOfChunks).push_back(c);
            }
        } else {
            mFreeChunks.push_back(c);
        }
    }<?php
grokit\sql_end_statement_table();
?>
;

    // cout << "\nLOADDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD" << mLastPage << endl;
/*
    // Now place the biggest free chunk at the end for every relation
    for (map<uint64_t, ChunkList*>::iterator it = mRelationIDToChunkList.begin(); it != mRelationIDToChunkList.end(); ++it) {
        uint64_t rel = it->first;
        ChunkList* l = it->second;
        uint64_t maxSizeIndex = -1;
        off_t maxSize = -1;
        for (uint64_t i = 0; i < (l->listOfChunks).size(); i++) {
            off_t startPg = ((l->listOfChunks)[i]->startPage);
            off_t sz = ((l->listOfChunks)[i]->size);
            off_t n = (l->listOfChunks)[i]->nextFreePage;
            if (startPg+sz-n > maxSize) {
                maxSize = startPg+sz-n;
                maxSizeIndex = i;
            }
        }
        assert(maxSizeIndex != -1);
        // Now place the maximum free space block at the end and remove from the middle
        (l->listOfChunks).push_back((l->listOfChunks)[maxSizeIndex]);
        (l->listOfChunks).erase((l->listOfChunks).begin() + maxSizeIndex);
    }
*/
    pthread_mutex_unlock(&mutex);
}

