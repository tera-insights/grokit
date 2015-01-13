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
#include "Debug.h"
#include "DiskPool.h"
#include "ChunkReaderWriter.h"
#include "DiskIOMessages.h"
#include "FileMetadata.h"

using namespace std;

extern EventProcessor executionEngine; // the global execution engine

// object hosted here
DiskPool globalDiskPool; // global object that manages the disk  pool

void DiskPool::Stop(){

}

TableScanID DiskPool::AddFile(string name, int numCols){
    // do we have the file started?
    TableScanID id(name);

    if (files.IsThere(id)){
        // yes, just return id and do nothing else
        return id;
    }

    ChunkReaderWriter file(name.c_str(), numCols, executionEngine);

    off_t numChunks = file.GetNumChunks();
    ClusterRangeList cRanges = file.GetClusterRanges();

    PDEBUG("Started %s stream with %d chunks and %d columns", name.c_str(), numChunks, numCols);

    // now start the chunkreader writer
    file.ForkAndSpin();

    // update the maps
    TableScanID cID = id; // copied before insert due to swap
    files.Insert(cID, file);
    sizes[id]=numChunks;
    clusterRanges[id] = cRanges;

    return id;
}

void DiskPool::StopFile(TableScanID id){
    // TODO
}


off_t DiskPool::NumChunks(TableScanID id){
    FATALIF( !files.IsThere(id), "Why are we asking about the number of chunks of a file not started?");
    SizeMap::iterator it = sizes.find(id);
    assert(it != sizes.end());
    return it->second;
}

auto DiskPool::ClusterRanges(TableScanID id) -> ClusterRangeList {
    FATALIF( !files.IsThere(id), "Why are we asking about the cluster ranges of a file not started?");
    auto it = clusterRanges.find(id);
    FATALIF(it == clusterRanges.end(),
        "No cluster range information found");
    return it->second;
}

void DiskPool::ReadRequest(ChunkID& id, WayPointID &requestor, bool useUncompressed,
        HistoryList &lineage, QueryExitContainer &dest,
        GenericWorkToken& token, SlotPairContainer& colsToProcess){

    // check if the token is forged
    FATALIF(token.Type() != DiskWorkToken::type, "I got a fake disk token in read");


    off_t chunkID = (int) id; // conversin to int
    TableScanID tId = id.GetTableScanId();

    FATALIF( !files.IsThere(tId), "Sending a read request to unknown file");

    EventProcessor& evProc = files.Find(tId);

    ChunkRead_Factory(evProc, requestor, chunkID, useUncompressed, lineage,
            dest, token, colsToProcess);

}

void DiskPool::WriteRequest(ChunkID& id, WayPointID &requestor, Chunk& chunk,
        HistoryList &lineage, QueryExitContainer &dest,
        GenericWorkToken& token, SlotPairContainer& colsToProcess
        ){

    // check if the token is forged
    FATALIF(token.Type() != DiskWorkToken::type, "I got a fake disk token in write");


    TableScanID tId = id.GetTableScanId();

    FATALIF( !files.IsThere(tId), "Sending a read request to unknown file");

    EventProcessor& evProc = files.Find(tId);

    ChunkWrite_Factory(evProc, requestor, chunk, lineage,
            dest, token, colsToProcess);

    // we have one more chunk, so increment the number
    sizes[tId]++;

}

void DiskPool::UpdateClusterRange(ChunkID& id, WayPointID &requestor,
    ClusterRange& range) {

    TableScanID tId = id.GetTableScanId();

    FATALIF( !files.IsThere(tId), "Sending cluster range update to unknown file");

    EventProcessor& evProc = files.Find(tId);

    ChunkClusterUpdate_Factory(evProc, requestor, id, range);
}

void DiskPool::Flush (TableScanID id) {
    EventProcessor& evProc = files.Find(id);
    Flush_Factory(evProc);
}

void DiskPool :: DeleteContent(std::string name) {
    TableScanID id(name);
    if( !files.IsThere(id) ) {
        FileMetadata::DeleteRelation(name);
    } else {
        EventProcessor& evProc = files.Find(id);
        DeleteContent::Factory(evProc);
    }
}

void DiskPool :: DeleteRelation(std::string name) {
    // do we have the file started?
    TableScanID id(name);

    if (files.IsThere(id)){
        FATAL("Attempting to delete a relation in use.");
    }

    FileMetadata::DeleteRelation(name);
}



