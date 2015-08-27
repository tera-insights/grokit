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
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "BStringIterator.h"
#include "Errors.h"
#include "DiskArray.h"
#include "Column.h"
#include "MMappedStorage.h"
#include "Chunk.h"
#include "BString.h"
#include "DistributedCounter.h"
#include "MmapAllocator.h"
#include "ChunkReaderWriterImp.h"
#include "Constants.h"
#include "ExecEngineData.h"
#include "EEExternMessages.h"
#include "Debug.h"


ChunkReaderWriterImp::ChunkReaderWriterImp(const char* _scannerName, uint64_t _numCols,
        EventProcessor& _execEngine):
    metadataMgr(_scannerName, _numCols), diskArray(DiskArray::GetDiskArray())
#ifdef  DEBUG_EVPROC
    ,EventProcessorImp(true, "ChunkReaderWriter")
#endif
{
    TableScanID __id(_scannerName);
    fileScannerId.swap(__id);

    execEngine.copy(_execEngine);

    nextRequest = 0; // counter to generate independent requests for all
    // disk jobs. Also counts how many requests we
    // processed since starting

    //priority for processing read chunks is higher than accepting new chunks
    RegisterMessageProcessor(MegaJobFinished::type, &ChunkRWJobDone, 3);
    RegisterMessageProcessor(ChunkRead::type, &ReadChunk, 2);
    RegisterMessageProcessor(ChunkWrite::type, &WriteChunk, 1);
    RegisterMessageProcessor(Flush::type, &FlushFunc, 4);
    RegisterMessageProcessor(DeleteContent::type, &DeleteContentFunc, 5);
    RegisterMessageProcessor(ChunkClusterUpdate::type, &ClusterUpdateFunc, 6);
}

uint64_t ChunkReaderWriterImp::NewRequest(){ return ++nextRequest; }

ChunkReaderWriterImp::~ChunkReaderWriterImp() {
}

MESSAGE_HANDLER_DEFINITION_BEGIN(ChunkReaderWriterImp, ChunkRWJobDone, MegaJobFinished){

    // Once the job is finished, flush the metadata
    PDEBUG("MESSAGE_HANDLER_DEFINITION_BEGIN(ChunkReaderWriterImp, ChunkRWJobDone, MegaJobFinished)");

    //identify the request that was finished
    off_t requestIdInitial = msg.requestId;

    // detele the distributed counter that got created in the DiskArrary
    delete msg.counter;

    // whatever request finished, we have to do the same thing: get the
    // hopping message from requests and send it to the execution engine
    KOff_t key(requestIdInitial);
    KOff_t dummy;
    CRWRequest req;
    evProc.requests.Remove(key, dummy, req);

    // chunk will be make readonly in the Table waypoint

    // and send it
    HoppingDataMsgMessage_Factory (evProc.execEngine, req.get_chunkID(), req.get_token(), req.get_hMsg());

}MESSAGE_HANDLER_DEFINITION_END


MESSAGE_HANDLER_DEFINITION_BEGIN(ChunkReaderWriterImp, ReadChunk, ChunkRead){
    off_t counter = 0;

    off_t _chunkId = msg.chunkID; // which chunk to read
    FATALIF( _chunkId>=evProc.metadataMgr.getNumChunks(), "A chunk not in the relation requested");

    DiskRequestDataContainer dRequests; // the page requests for each thread

    // no numa for now
    // uint64_t numaNode = msg.chunk.GetNumaNode();

    // create the chunk
    Chunk chunk;

    //create bitmap
    QueryID queries = QueryExitsToQueries(msg.dest);

    MMappedStorage bitStore;
    Column outBitCol(bitStore);
    assert(evProc.metadataMgr.getNumTuples(_chunkId));
    BStringIterator outQueries (outBitCol, queries, evProc.metadataMgr.getNumTuples(_chunkId));
    outQueries.SetFragmentsTuples(evProc.metadataMgr.getFragmentsTuples(_chunkId));
    outQueries.Done();
    //outQueries.Done(outBitCol);
    //chunk.SwapBitmap(outBitCol);
    chunk.SwapBitmap(outQueries);

    // go through all the columns and produce the allocation and the
    // disk requests
    // pay attentention to the special QueryIDs columns
    FOREACH_TWL(col, msg.colsToProcess){
        SlotID index = col.second; // phisical column
        SlotID chkSlot = col.first; // logical column

        off_t startPage;
        off_t sizePages;
        off_t sizeCompressed;
        off_t sizeUncompressed;

        if (msg.useUncompressed || evProc.metadataMgr.getSizeBytesCompr(_chunkId, index) == 0 ||
                ( evProc.metadataMgr.getSizeBytesCompr(_chunkId, index) > .75 * evProc.metadataMgr.getSizeBytes(_chunkId, index)) ){
            // uncompressed columns
            startPage = evProc.metadataMgr.getStartPage(_chunkId, index);
            sizePages = evProc.metadataMgr.getSizePages(_chunkId, index);
            sizeCompressed = 0;
            sizeUncompressed = evProc.metadataMgr.getSizeBytes(_chunkId, index);
        } else { // compressed columns
            startPage = evProc.metadataMgr.getStartPageCompr(_chunkId, index);
            sizePages = evProc.metadataMgr.getSizePagesCompr(_chunkId, index);
            sizeCompressed = evProc.metadataMgr.getSizeBytesCompr(_chunkId, index);
            sizeUncompressed = evProc.metadataMgr.getSizeBytes(_chunkId, index);
        }

        // allocate memory
        void* data = mmap_alloc(PAGES_TO_BYTES(sizePages), 1/*, numaNode*/);
        //create the column and load it into the chunk
        MMappedStorage colStorage(data, sizeUncompressed, sizeCompressed );
        Column newColumn(colStorage);
        newColumn.SetFragments(evProc.metadataMgr.getFragments(_chunkId, index));

        chunk.SwapColumn(newColumn, chkSlot);

        // now create the disk requests
        counter = counter + sizePages;
        DiskRequestData req (startPage, sizePages, data);

        dRequests.Append(req);
    }END_FOREACH

    off_t requestID = evProc.NewRequest();

    // place chunk in HoppingMessage and message in RequestsMap
    ChunkContainer chkContainer(chunk);
    HoppingDataMsg result (msg.requestor, msg.dest, msg.lineage, chkContainer);
    CRWRequest req(_chunkId, result, msg.token);
    KOff_t key(requestID);
    evProc.requests.Insert(key, req);

    evProc.totalPages+=counter;

    EventProcessor copy;
    copy.copy(evProc.myInterface);

    // send the job to the disk array
    DiskOperation_Factory(evProc.diskArray, requestID, READ, copy, dRequests);

}MESSAGE_HANDLER_DEFINITION_END

/** helper function to translate raw lists to disk requests
  will be used twice for compressed and uncompressed data

  returns the next empty page: startPage-return = sizePages
  */

static off_t RawListToDiskRequest(off_t startPage,
        RawStorageList& in, DiskRequestDataContainer& out){

    off_t curr = startPage;

    FOREACH_TWL(el, in){
        // uint64_t bytes = in.Current().sizeInBytes;  // not used
        off_t pages = el.sizeInPages;
        void* data = el.data;
        DiskRequestData req(curr,	pages, data);
        out.Append(req);
        curr+=pages;
    }END_FOREACH

    return curr;
}

MESSAGE_HANDLER_DEFINITION_BEGIN(ChunkReaderWriterImp, WriteChunk, ChunkWrite){
    off_t counter = 0;

    // cout << "Got request" << endl;

    DiskRequestDataContainer dRequests; // the page requests for each thread

    // extract the number of tuples from BStringIterator
    //Column bString;
    BStringIterator bSIter;
    msg.chunk.SwapBitmap(bSIter);
    //msg.chunk.SwapBitmap(bString);
    //BStringIterator bSIter(bString);
    uint64_t numTuples = bSIter.GetNumTuples();

    // If there are no tuples to write, just toss the chunk away
    if( numTuples == 0 ) {
        // Send the acknowledgement for the chunk
        ChunkContainer chkContainer(msg.chunk);
        HoppingDataMsg result (msg.requestor, msg.dest, msg.lineage, chkContainer);

        HoppingDataMsgMessage_Factory (evProc.execEngine, 0, msg.token, result);

        return;
    }

    // translate the request into a request for ChunkReaderWriter
    // the chunk is taken apart and broken into columns
    // space is allocated in metadata for each column
    // the space and the columns are sent to
    off_t _chunkId = evProc.metadataMgr.startNewChunk(numTuples, msg.colsToProcess.Length(), bSIter.GetFragmentsTuples());
#ifdef DEBUG
    FATALIF(msg.colsToProcess.Length()==0, "No columns received for chunkID %d, numtuples = %d", _chunkId, numTuples);
#endif

    uint64_t nextCol = 0;
    // go through all the columns and produce the allocation and the
    // disk requests
    FOREACH_TWL(colD, msg.colsToProcess){
        SlotID slot = colD.first; // logical column
        SlotID index = colD.second; // phisical column

#ifdef DEBUG
        // Debug information
        IDInfo l;
        (const_cast<SlotID&>(slot)).getInfo(l);
        IDInfo r;
        (const_cast<SlotID&>(index)).getInfo(r);
        PDEBUG("Logical column = %s, Physical column = %s", (l.getName()).c_str(), (r.getName()).c_str());
#endif

        FATALIF(index != nextCol, "We should see the coluns in order here with no skips");
        nextCol++;

        Column col;
        if (slot!=BITSTRING_SLOT){ // regular column
            msg.chunk.SwapColumn(col, slot);
        } else { // have to write the actual Bitstring
            bSIter.Done(col);
        }

        off_t sizePages = 0;
        off_t sizePagesCompr = 0;
        off_t startPage = 0;
        off_t startPageCompr = 0;
        Fragments frag;

        if (col.IsValid()) {
            // get the size needed in pages
            sizePages = col.GetUncompressedSizePages();
            sizePagesCompr = col.GetCompressedSizePages();
            frag = col.GetFragments();
            startPage = evProc.diskArray.AllocatePages(sizePages,
                    evProc.metadataMgr.getRelID());
            startPageCompr = evProc.diskArray.AllocatePages(sizePagesCompr,
                    evProc.metadataMgr.getRelID());
        }

        counter+=sizePages+sizePagesCompr;

        // bookkeeping for the column
        if (col.IsValid()) {
            evProc.metadataMgr.addColumn(startPage,
                    col.GetUncompressedSizeBytes(),
                    sizePages,
                    startPageCompr,
                    col.GetCompressedSizeBytes(),
                    sizePagesCompr,
                    frag );
        } else {
            evProc.metadataMgr.addColumn(0,
                    0,
                    0,
                    0,
                    0,
                    0,
                    frag); // dummy fragment
        }

        // get now the memory from the column
        RawStorageList rawList;
        if (sizePages != 0) {
            col.GetUncompressed(rawList);
            off_t end = RawListToDiskRequest(startPage, rawList, dRequests);

            FATALIF( (end-startPage) > sizePages,
                    "Sizes of used and allocated disk space do not match. startP=%d\tend=%d\tsize=%d\n",
                    (uint64_t)startPage, (uint64_t)end, (uint64_t)sizePages);
        }

        if (sizePagesCompr != 0) {
            col.GetCompressed(rawList);
            off_t end = RawListToDiskRequest(startPageCompr, rawList, dRequests);

            FATALIF( (end-startPageCompr) > sizePagesCompr,
                    "Sizes of used and allocated disk space do not match");
        }

        // put column back
        msg.chunk.SwapColumn(col, slot);
    }END_FOREACH


    evProc.metadataMgr.finishedChunk();
    evProc.totalPages+=counter;
    off_t requestID = evProc.NewRequest();

    KOff_t key(requestID);
    ChunkContainer chkContainer(msg.chunk);
    HoppingDataMsg result (msg.requestor, msg.dest, msg.lineage, chkContainer);
    CRWRequest req(_chunkId, result, msg.token);
    evProc.requests.Insert(key, req);

    EventProcessor copy;
    copy.copy(evProc.myInterface);

    // send the job to the disk array
    DiskOperation_Factory(evProc.diskArray, requestID, WRITE, copy, dRequests);

}MESSAGE_HANDLER_DEFINITION_END


MESSAGE_HANDLER_DEFINITION_BEGIN(ChunkReaderWriterImp, FlushFunc, Flush){
    evProc.metadataMgr.Flush();
}MESSAGE_HANDLER_DEFINITION_END

MESSAGE_HANDLER_DEFINITION_BEGIN(ChunkReaderWriterImp, DeleteContentFunc, DeleteContent) {
    evProc.metadataMgr.DeleteContent();
}MESSAGE_HANDLER_DEFINITION_END

MESSAGE_HANDLER_DEFINITION_BEGIN(ChunkReaderWriterImp, ClusterUpdateFunc, ChunkClusterUpdate) {
    ChunkID id = msg.chunkID;
    FileMetadata::ClusterRange range = msg.range;

    off_t chunkNum = id.GetValue();

    evProc.metadataMgr.updateClusterRange(chunkNum, range);
}MESSAGE_HANDLER_DEFINITION_END
