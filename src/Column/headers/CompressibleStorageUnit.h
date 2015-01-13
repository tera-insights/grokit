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

#ifndef COMPRESS_STOR_UNIT_H
#define COMPRESS_STOR_UNIT_H

#include <cstring>
#include <cstdlib>

#include <iostream>

#include "MmapAllocator.h"
#include "Errors.h"
#include "Constants.h"
#include "StorageUnit.h"

// Compression algorithm
#define QLZ_COMPRESSION_LEVEL 3
#define QLZ_STREAMING_BUFFER 100000
#define QLZ_EXTRA_SPACE 1000 /** extra space qlz needs to compress */
#include "quicklz.h"

//#define COMPRESS_ALL_AT_ONCE 1

// this structure is used by the mmapped storage to store all of its chunks of memory in compressed form
// which is received from user
struct CompressedStorageUnit {

    private:
        // start location of decompressed data
        char* decompressedBytes;
        int nextDecompress; // last byte decompressed in decompressedBytes buffer

        // start location of compressed data
        char *compressedBytes;
        int nextCompress; // last byte compressed in compressedBytes buffer up to which we are done with decompression

        // total decompressed size
        int decompressedSize;
        // total compressed size
        int compressedSize;

        // how much we decompressed in last call
        int lastDecompressedLength;

        // state needs to be passed to compress and decompress quicklz functions
        qlz_state_compress *state_compress;

        // These are states used by quicklz compresser and decompresser
        qlz_state_decompress *state_decompress;

        // numa node
        int numa;

    public:

        /// -------------------- Functions ------------------//

        // numa not needed here since this is supposed to be used only for swapping
        CompressedStorageUnit ();

        // decompressed data constructor
        CompressedStorageUnit(int _dataSize, int _numa = 0);
        // compressed data constructor, where is the sister storage unit
        // that accesses teh decompressed data
        CompressedStorageUnit(char* _data, int _dataSize, int _compressedSize, StorageUnit& where, int _numa = 0);

        // states are local to object and are created and destroyed with object.
        // in case of deep and shallow copies, they are created new and not copied
        ~CompressedStorageUnit () {
            if (state_decompress) free(state_decompress), state_decompress = NULL;
            if (state_compress) free(state_compress), state_compress = NULL;
        }

        void swap (CompressedStorageUnit &withMe);

        // funciton to add info on raw storage of compressed data to a list
        void AddCompressedRawStorage(RawStorageList& out);

        // Compress a Storage Unit and add it to compressed
        void CompressThisStorageUnit(StorageUnit& unit);

        // decompress at least up to given position, but actual decompression could be much
        // more depending on decompression algorithm used by quicklz stream decompression
        int DecompressUpTo(int decompress_position);

        // same function as above but the decompression is always at the
        // beggining of the buffer. The new decompressed data overrides the
        // old one
        // Note: the caller has to know that we always write from the
        // beggining of the buffer, not continuously
        int DecompressInPlace(int posToStartFrom, int decompress_position);

        // does a simple copy
        void copy (CompressedStorageUnit &fromMe);

        // does a deep copy
        //void Detach(CompressedStorageUnit &withMe);
        void CreateDeepCopy(CompressedStorageUnit &withMe);

        // frees the memory pointed to
        void Kill ();


        // Get the size of original decompressed data reading the header info in the compressed data
        int GetDecompressedSize();

        // Get the current compressed size reading the header of compressed data
        int GetCompressedSize();

        bool GetIsCompressed(){ return compressedSize>0; }
};

//////////////////// inline Definitions ///////////////////////////////

// This gives raw handle of data to the external world and it may happen that this raw
// data storage is used to create new Column and in that case both column will destory
// the storage and hence double free !! Take care.
inline 	void CompressedStorageUnit::AddCompressedRawStorage(RawStorageList& out){
    // nextCompressed has the size of compressed not compressedSize
    // shich should be 0
    RawStorageUnit tmp(compressedBytes, BYTES_TO_PAGES(nextCompress), nextCompress);
    out.Append(tmp);
}

// For default constructor as below, it is very important to set vars to NULL, otherwise
// they all will be copied in shallow/deep copy unnecessarily
inline
CompressedStorageUnit::CompressedStorageUnit() : decompressedBytes(NULL), nextDecompress(0),
    compressedBytes(NULL), nextCompress(0), decompressedSize(0), compressedSize(0),
    state_compress(NULL), state_decompress(NULL){}

    inline /* data is decompressed, we'll compress */
    CompressedStorageUnit::CompressedStorageUnit(int _decompressedSize, int _numa):
        decompressedBytes(NULL),
        nextDecompress(0),
        compressedBytes((char*) mmap_alloc(_decompressedSize + QLZ_EXTRA_SPACE, _numa)),
        nextCompress(0),
        decompressedSize(_decompressedSize),
        compressedSize(0),
        state_compress((qlz_state_compress *)malloc(sizeof(qlz_state_compress))),
        state_decompress(NULL),
        numa(_numa)
{
    // zero out the state_compress
    memset(state_compress, 0, sizeof(qlz_state_compress));
}

inline /** data is compressed */
CompressedStorageUnit::CompressedStorageUnit(char* _data, int _decompressedSize,
        int _compressedSize, StorageUnit& where, int _numa):
    decompressedBytes((char*) mmap_alloc(_decompressedSize, _numa)),
    nextDecompress(0),
    compressedBytes(_data),
    nextCompress(0),
    decompressedSize(_decompressedSize),
    //decompressedSize(qlz_size_decompressed(_data)),
    compressedSize(_compressedSize),
    state_compress(NULL),
    state_decompress((qlz_state_decompress *)malloc(sizeof(qlz_state_decompress))),
    numa(_numa)
{
    // create sister storage unit
    where.bytes=decompressedBytes;
    where.start=0;
    where.end=decompressedSize - 1;
    // zero out the state_compress
    memset(state_decompress, 0, sizeof(qlz_state_decompress));

    assert(decompressedSize == qlz_size_decompressed(_data));
}

inline
int CompressedStorageUnit::GetDecompressedSize() {
    return decompressedSize;
}

inline
int CompressedStorageUnit::GetCompressedSize () {
    return compressedSize;
}

inline
void CompressedStorageUnit::swap (CompressedStorageUnit &withMe) {
    char storage[sizeof (CompressedStorageUnit)];
    memmove (storage, this, sizeof (CompressedStorageUnit));
    memmove (this, &withMe, sizeof (CompressedStorageUnit));
    memmove (&withMe, storage, sizeof (CompressedStorageUnit));
}

inline
void CompressedStorageUnit::CreateDeepCopy(CompressedStorageUnit &withMe) {

    // create shallow copy of everything, then correct desired one's
    memmove (&withMe, this, sizeof (CompressedStorageUnit));

    // create a new state (no need to deep copy), if previous one existed
    if (state_decompress) {
        withMe.state_decompress = (qlz_state_decompress *)malloc(sizeof(qlz_state_decompress));
        memmove(withMe.state_decompress, state_decompress, sizeof(qlz_state_decompress));
    }

    // create a new state (no need to deep copy), if previous one existed
    if (state_compress) {
        withMe.state_compress = (qlz_state_compress *)malloc(sizeof(qlz_state_compress));
        memmove(withMe.state_compress, state_compress, sizeof(qlz_state_decompress));
    }

    // create a deep copy of decompressed bytes
    if (decompressedBytes) {
        // If we have compressed data, we always allocate decompressed bytes in advance
        withMe.decompressedBytes = (char*) mmap_alloc (decompressedSize, withMe.numa);

        // We don't need to copy decompressed bytes because it may be partially decompressed.
        // In case it is already fully decompressed, probably we might want to pay cost of
        // decompressing it again instead of copy operation below.
        // And for decompress in place, we dont need this copy anyway since it is always
        // recomputed, and mostly we will be decompressing in place in future.
        //memmove(withMe.decompressedBytes, decompressedBytes, nextDecompress);

        // reset the variables so that decompression can start again
        withMe.nextDecompress = 0;
        withMe.nextCompress = 0;
        withMe.lastDecompressedLength = 0;
    }

    // create a deep copy of compressed bytes
    if (compressedBytes) {
        withMe.compressedBytes = (char*) mmap_alloc (decompressedSize+QLZ_EXTRA_SPACE, withMe.numa);
        memmove(withMe.compressedBytes, compressedBytes, nextCompress);
    }
}

// This works in streaming mode internally, see quicklz.h flags
inline
int CompressedStorageUnit::DecompressUpTo(int decompress_position) {
    // we assume that the decompression process proceeds linearly and
    // does not get restarted
    FATALIF( nextCompress==0 && nextDecompress!=0, "The decompression process is not linear");

    // we go in a loop until we stream-decompressed enough
    while (decompress_position > nextDecompress){
        int csize = qlz_size_compressed(compressedBytes+nextCompress);

        nextDecompress += qlz_decompress(compressedBytes+nextCompress,
                decompressedBytes+nextDecompress,
                state_decompress);
        nextCompress+=csize;
    }

    return nextDecompress;
}

// This works in streaming mode internally, see quicklz.h flags
inline
int CompressedStorageUnit::DecompressInPlace(int posToStartFrom, int decompress_position) {

    // In case we restart the iteration from start, we need to reset all variables.
    if (posToStartFrom == 0) {
        nextDecompress = 0;
        nextCompress = 0;
        lastDecompressedLength = 0;
    }

    // Compute the last portion of buffer length which is already decompressed, but was not
    // used in last shot. We need to keep things contiguous starting from posToStartFrom.
    // We will start decompressing from startbuffer + dNext, since we already copied up to dNext
    // in the start decompressed buffer. SO overall, just copy last unused portion to the start
    // and start decompressing after that.
    int dNext = nextDecompress - posToStartFrom;


    // If it is positive amount, do the copy. It should not be negative until posToStart takes
    // sudden forward jump arbitrarily (which could be the case in deep copy where we assume
    // nothing is decompressed, but to user he has to start from posToStart, for us it is a
    // forward jump). There should not be case where it takes backward jump, because in that case
    // dNext will be too high, and our assumption is that it will not be high (we only are supposed
    // to copy last remaining portion). If it is too high, it may exceed the start point and hence
    // corruption !!
    if (dNext > 0) {

        // If this is not the case, our source will go beyond the start of decompressed buffer. In theory
        // check should be (lastDecompressedLength/2 > dNext), since we are using memcpy, and hence source
        // and destination should not overlap. This will fail only if posToStart takes back jump, which is
        // not possible since all iterators ensures it.
        assert(lastDecompressedLength > dNext);

        char* source = decompressedBytes + lastDecompressedLength - dNext;
        // copy the last remaining portion of data to create continuous pool
        // As we see, source and destination overlap here and it is recommended
        // to use memmove for those cases, but I assume it should not be problem
        // here as they may not overlap in almost all cases. Few bytes of end part
        // is copied to the start
        //memcpy(decompressedBytes, source, dNext);
        memmove(decompressedBytes, source, dNext);
        //} else if (dNext < 0) { // comment it since we dont really need to set any other var
} else {
    // posToStartFrom has taken forward jump, did not arrive in continuous fashion
    // But in this case, we can assume all variables are set to zero already, hence
    // we will be decompressing all from start.
    dNext = 0;
}
// we go in a loop until we stream-decompressed enough, this will be done
// everytime posToStartFrom is starting from zero again. Hence we have
// this drawback to decompress in every pass once we have fully decompressed
// in first pass. But this is significantly useful in single pass case.
while (decompress_position > nextDecompress){
    // printf("\ndnexxxxxxxxxxxxxttttttttttttttttttttttttttttttttttt of decompressed buffer = %d %lx\n", dNext, decompressedBytes);

    int csize = qlz_size_compressed(compressedBytes+nextCompress);


    int dsize = qlz_decompress(compressedBytes+nextCompress,
            decompressedBytes+dNext,
            state_decompress);
    nextCompress+=csize;
    nextDecompress +=dsize;
    dNext +=dsize;
}
// remember the length of our buffer to be used in next iteration copy operation
lastDecompressedLength = dNext;

return nextDecompress;
}

// does a simple copy
inline
void CompressedStorageUnit::copy (CompressedStorageUnit &fromMe) {

    // do shallow copy and then correct desired states
    memmove (this, &fromMe, sizeof (CompressedStorageUnit));

    // create a new state (no need to deep copy), if previous one existed
    if (fromMe.state_decompress) {
        state_decompress = (qlz_state_decompress *)malloc(sizeof(qlz_state_decompress));
        memmove(state_decompress, fromMe.state_decompress, sizeof(qlz_state_decompress));
    }

    // create a new state (no need to deep copy), if previous one existed
    if (fromMe.state_compress) {
        state_compress = (qlz_state_compress *)malloc(sizeof(qlz_state_compress));
        memmove(state_compress, fromMe.state_compress, sizeof(qlz_state_decompress));
    }
}

// frees the memory pointed to
inline
void CompressedStorageUnit::Kill () {
    // decompressBytes are taken over by StorageUnit
    if (compressedBytes != NULL) 	mmap_free (compressedBytes), compressedBytes = NULL;
    //if (decompressedBytes != NULL) 	mmap_free (decompressedBytes), decompressedBytes = NULL;
}

inline
void CompressedStorageUnit::CompressThisStorageUnit(StorageUnit& unit){
    // we chop the storage unit into COMPRESSION_UNIT parts that we
    // compress. This way, when we decompress, we only get this much
    // thus conserving L2 cache
    //
    // Note: we assume that the ritht amount of space was allocated
    // already in constructor
    int num = 0; // how much we compressed from this unit
#ifndef COMPRESS_ALL_AT_ONCE
    int sizeToCompress = COMPRESSION_UNIT;
#else
    int sizeToCompress = unit.Size();
#endif
    int size = unit.Size();
    while (num < size) {
        if (num + sizeToCompress > size)
            sizeToCompress = size - num;
        int cSize = qlz_compress((const char*)unit.bytes + num, compressedBytes+nextCompress,
                sizeToCompress, state_compress);
        nextCompress += cSize;
        num += sizeToCompress;
    }
    // set the compressedSize to what we compressed so far
    compressedSize = nextCompress;
}

#endif // COMPRESS_STOR_UNIT_H
