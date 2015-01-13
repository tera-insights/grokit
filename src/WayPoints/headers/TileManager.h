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
#ifndef _TileManager_H_
#define _TileManager_H_

#include <map>
#include <set>
#include <vector>
#include "ChunkID.h"
#include "HashTableMacros.h"

//#define NUM_FRAGMENTS (1<<NUM_FRAGMENT_BITS)
// This is what we use while partitioning one chunk into multiple chunks. Each chunk consists of this
// many fragments range
#define NUM_FRAGMENTS_PER_SLICE 1
// This is used to slice out small tiles out of floor. m lhs chunks x m rhs chunks of size
//#define SUBFLOOR_SIZE 4
#define SUBFLOOR_SIZE 1

class ChunkMetaData {

    // Keep bucket to tiles mapping. These tiles are created from a floor which has several lhs and rhs
    // chunks (smaller parted chunks covering fragments). Once read, delete from this map. If all tiles
    // are deleted from some bucket, remove the bucket.
    // If we get unprocessed tile back, add into this bucket, to be read later
    // This bucket is a pseudo bucket, as we have used smaller portion of bucket to create floor
    // Note that each tile is vector of lhs and vector of rhs chunks
    typedef std::map<__uint64_t, std::multimap<std::vector<ChunkID>, std::vector<ChunkID> > > BucketToFloorMap;
    BucketToFloorMap myBucketToFloorMap;

    // Chunks written on disk
    // Please note that the chunks which are going to disk may not come back, but smaller fragments of
    // them have to come back to constitute a sub floor
    typedef  std::map<int, std::set<ChunkID> > BucketToChunkMap;
    BucketToChunkMap leftChunksToDisk;
    BucketToChunkMap rightChunksToDisk;

    // Here we divide buckets into many smaller buckets. Each bigger bucket above is subdivided into
    // many smaller buckets depending on how many fragments groups we want to create.
    // Number of smaller buckets per big bucket = MAX_FRAGMENTS / NUM_FRAGMENTS_PER_CHUNK
    // Each smaller bucket holds several Chunks of same fragment range
    typedef  std::map<__uint64_t, std::set<ChunkID> > BucketToChunkMapSmall;
    BucketToChunkMapSmall leftChunksToDiskSmaller;
    BucketToChunkMapSmall rightChunksToDiskSmaller;

    bool isSlicedAlready;
    int numFragments;
public:

    ChunkMetaData ();

    ~ChunkMetaData ();

    // This is called before sending chunk to disk for bookkeeping
    void ChunkToDisk (int bucket, ChunkID chunkID, bool isLeft);

    // This is called once we are done writing and ready to produce chunks
    void DoneWriting();
    void SetNumFragments(int _num) {
        if (numFragments != 0) {
            FATALIF(numFragments != _num, "Number of fragments of subsequent chunks are not same !!!");
            return;
        }
        numFragments = _num;
    }
    int GetNumFragments() {return numFragments;}

    // Get the tiles to work on, false if no tiles but it can become true later on so keep checking availability
    bool GetTile (__uint64_t& bucket, std::vector<ChunkID>& lhs, std::vector<ChunkID>& rhs);
    // get the floor and also bucket number
    bool GetFloor (__uint64_t& bucket, std::vector<ChunkID>& lhs, std::vector<ChunkID>& rhs);
    // get the slice of the floor and also bucket number
    bool GetSlice (__uint64_t& bucket, std::vector<ChunkID>& lhs, std::vector<ChunkID>& rhs);
    // using bucket, give me all tiles belonging to that bucket
    std::multimap<std::vector<ChunkID>, std::vector<ChunkID> > GetAllTiles (__uint64_t bucket);

    // if tile is returned, add it into the floor to send back again at later time
    void UnprocessedTile(__uint64_t bucketID, std::vector<ChunkID> chunkLHS, std::vector<ChunkID> chunkRHS);
    void UnprocessedFloor(__uint64_t bucketID, std::set<ChunkID> chunkLHS, std::set<ChunkID> chunkRHS);
    void UnprocessedSlice(__uint64_t bucketID, std::vector<ChunkID> chunkLHS, std::vector<ChunkID> chunkRHS);

    // Did we produce all tiles (all combinations of pair of chunks from same bucket)
    bool IsEmpty();

    // For debug purpose
    void PrintAllTiles ();
    void PrintAllSlices ();
    void PrintFloor (std::map<ChunkID, std::set<ChunkID> >& floor);

private:

    // helper function
    void Insert (int bucket, ChunkID chunkID, std::map<int, std::set<ChunkID> >& ChunksToDisk);

    // Sub divide larger chunks to smaller chunks for reading, because we read only small bucket floors
    void SubDivide();
    void SubDivide (BucketToChunkMap& ChunksToDisk, BucketToChunkMapSmall& ChunksToDiskSmaller);

};

#endif
