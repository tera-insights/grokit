//
//  Copyright 2013 Christopher Dudley
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

#include "TwoWayList.h"
#include "EfficientMap.h"
#include "GIWayPointImp.h"
#include "CPUWorkerPool.h"
#include "Profiling.h"
#include "DictionaryManager.h"
#include "Logging.h"
#include "Swap.h"
#include "Stl.h"

using namespace std;

const double GIWayPointImp::CACHE_INTERVAL = 0.2;

GIWayPointImp :: GIWayPointImp () :
    WayPointImp(),
    open_streams(),
    my_files(),
    myExits(),
    num_open_streams(0),
    num_chunks_out(0),
    num_chunks_in_flight(0),
    next_chunk_no(0),
    tokensRequested(0),
    tID(),
    last_cache_send(0.0),
    chunkMap(),
    chunkCache()
{
    PDEBUG( "GIWayPointImp :: GIWayPointImp()" );
    // Don't set the number of work tokens here, do that when we know how many
    // streams we have.
    num_open_streams = 0;
    num_chunks_out = 0;
    next_chunk_no = 0;

    // TEMPORARY
    last_cache_send = 0.0;
}

GIWayPointImp :: ~GIWayPointImp () {
    PDEBUG("GIWayPointImp :: ~GIWayPointImp()" );
    // Nothing
}

void GIWayPointImp :: RequestTokens(){
    PDEBUG ("GIWayPointImp :: RequestTokens ()");

    int noReq = 0;
    if( chunkCache.Length() == 0 ) {
        noReq = tasks.Length() - tokensRequested;
    }
    else {
        noReq = chunkCache.Length() - tokensRequested;
        noReq = noReq < 0 ? 0 : noReq;
    }

    FATALIF (noReq < 0, "GI somehow attempting to request a negative number of tokens.");

    // is that too many?
    //WARNINGIF(noReq > dblBuf, "Too many request: %d\n", noReq);

    // queue up some more work requests
    // one for each element of the list
    for (int i=0; i<noReq; i++) {
        tokensRequested++;
        RequestTokenDelayOK (CPUWorkToken::type, limiter.GetMinStart());
    }
}

void GIWayPointImp :: SendCachedChunk( CachedChunk& chunk ) {
    PDEBUG( "GIWayPointImp :: SendCachedChunk()" );
    QueryExitContainer &whichOnes = chunk.get_whichExits();
    HistoryList &lineage = chunk.get_lineage();
    ChunkContainer &chunkCont = chunk.get_myChunk();

    num_chunks_in_flight++;

    // Send the message
    SendHoppingDataMsg( whichOnes, lineage, chunkCont );
}

void GIWayPointImp :: TypeSpecificConfigure( WayPointConfigureData &configData ) {
    PDEBUG("GIWayPointImp :: TypeSpecificConfigure()");

    GIConfigureData tempConfig;
    tempConfig.swap( configData );

    // Store query exits
    myExits.swap(tempConfig.get_queries());

    // Invent a tID for ourselves.
    TableScanID newID(this->GetName().c_str());
    tID.swap(newID);

    my_files = tempConfig.get_files();
}

void GIWayPointImp :: SetUpStreams() {
    // For each string in the input, create a new task
    off_t stream_id = 0;
    FOREACH_STL(file, my_files) {
        off_t my_stream_id = stream_id++;
        GIStreamInfo nInfo( file, my_stream_id );
        GLAState nState;

        GIStreamProxy sProxy = nInfo.get_proxy();
        GITask nTask( sProxy, nState );
        tasks.Append(nTask);
        ++num_open_streams;

        LOG_ENTRY_P(1, "GI Stated for File %s", file.c_str());

        // Add stream to mapping
        StreamKey key(my_stream_id);
        open_streams.Insert(key, nInfo);
    } END_FOREACH;
}

void GIWayPointImp :: RequestGranted( GenericWorkToken &returnVal ) {
    PDEBUG("GIWayPointImp :: RequestGranted()");

    CPUWorkToken myToken;
    myToken.swap(returnVal);

    --tokensRequested;

    // If we have chunks cached, serve those first and return the token.
    if( chunkCache.Length() > 0 ) {
        double curTime = global_clock.GetTime();

        if( curTime > last_cache_send + CACHE_INTERVAL ) {
            CachedChunk myChunk;
            chunkCache.MoveToStart();
            chunkCache.Remove( myChunk );

            // Send cached chunk
            SendCachedChunk( myChunk );
            last_cache_send = curTime;
        }

        GiveBackToken( myToken );

        if (num_chunks_in_flight == 0 && tokensRequested == 0) {
          // Need to request at least one token to prevent deadlocking
          tokensRequested++;
          RequestTokenDelayOK (CPUWorkToken::type, limiter.GetMinStart());
        }

        return;
    }

    // Do we even have some work to do?
    // We *should* always have some work at this point.
    FATALIF( tasks.Length() == 0, "GI got a token but had no work to do!" );

    // Get a task from the list
    GITask task;
    tasks.MoveToStart();
    tasks.Remove(task);

    // Set up the lineage, which we have to create from scratch
    QueryExitContainer myOutputExits;
    myOutputExits.copy( myExits );

    QueryIDSet queriesCovered;
    FOREACH_TWL(iter, myOutputExits) {
        queriesCovered.Union(iter.query);
    } END_FOREACH;

    ChunkID cID(next_chunk_no++, tID);
    GIHistory myHistory( GetID(), cID, task.get_stream().get_file_name() );
    HistoryList tempList;
    tempList.Insert( myHistory );

    // Set up the work description
    GIProduceChunkWD workDesc( task.get_gi(), task.get_stream(), queriesCovered );

    WorkFunc myFunc = GetWorkFunction( GIProduceChunkWorkFunc::type );
    WayPointID tempID = GetID();
    myCPUWorkers.DoSomeWork( tempID, tempList, myOutputExits, myToken, workDesc, myFunc );

    RequestTokens();
}

void GIWayPointImp :: ProcessHoppingUpstreamMsg( HoppingUpstreamMsg &message ) {
    PDEBUG("GIWayPointImp :: ProcessHoppingUpstreamMsg()");

    CONVERT_SWAP(message.get_msg(), myMessage, StartProducingMsg);

    FATALIF(!tasks.IsEmpty(), "GIWP got start producing message with streams still open!");

    QueryExit queryToStart = myMessage.get_whichOne();
    cout << "About to start ";
    queryToStart.Print();
    cout << "\n";

    SetUpStreams();

    RequestTokens();
}

void GIWayPointImp :: ProcessDropMsg( QueryExitContainer &whichExits, HistoryList &lineage ) {
    PDEBUG("GIWayPointImp :: ProcessDropMsg()");

    PROFILING2_INSTANT("chn", 1, GetName());

    num_chunks_in_flight--;

    // Get chunk from mapping
    EXTRACT_HISTORY_ONLY( lineage, myHistory, GIHistory );
    ChunkID cID = myHistory.get_whichChunk();
    myHistory.swap(lineage.Current());

    ChunkContainer chunkCont;
    FATALIF( ! chunkMap.IsThere( cID ), "Got drop for chunk I don't know about!" );
    chunkCont.copy( chunkMap.Find( cID ) );

    CachedChunk cCache(chunkCont, lineage, whichExits);
    chunkCache.Append(cCache);

    RequestTokens();
}

void GIWayPointImp :: DoneProducing( QueryExitContainer &whichOnes, HistoryList &history,
        int result, ExecEngineData &data ) {
    PDEBUG("GIWayPointImp :: DoneProducing()");

    GIHistory myHistory;
    history.MoveToStart();
    myHistory.swap(history.Current());
    string file = myHistory.get_file();
    ChunkID cID = myHistory.get_whichChunk();

    myHistory.swap(history.Current());

    GIProduceChunkRez tempResult;
    tempResult.swap(data);

    // For the chunk that goes out
    ChunkContainer &chkCont = tempResult.get_chunk();

    // DEBUG
    //Chunk &c = chkCont.get_myChunk();
    //printf("Number of Tuples: %lu\n", c.GetNumTuples() );

    ChunkContainer contCopy;
    contCopy.copy(chkCont);
    // Replace data with a chunk container
    data.swap(chkCont);
    num_chunks_out++;
    num_chunks_in_flight++;

    // Add chunk to mapping
    chunkMap.Insert(cID, contCopy);

    // Get the stream and GI from the result.
    GLAState& gi = tempResult.get_gi();
    GIStreamProxy& stream = tempResult.get_stream();

    if( result == 1 ) {
        // Done processing this file
        num_open_streams--;

        // Remove this stream from the mapping.
        off_t id_no = stream.get_id();
        StreamKey id_no_key(id_no);
        StreamKey key;
        GIStreamInfo sInfo;
        open_streams.Remove(id_no_key, key, sInfo);

        LOG_ENTRY_P(2, "GI %s finished processing file %s.", GetName().c_str(), stream.get_file_name().c_str());
    } else {
        // Repackage the GI and stream into a task and add it to the list
        GITask nTask(stream, gi);
        tasks.Append(nTask);
    }

    RequestTokens();
}

void GIWayPointImp :: ProcessAckMsg( QueryExitContainer &whichExits, HistoryList &lineage ) {
    PDEBUG("GIWayPointImp :: ProcessAckMsg()");

    // Make sure that the HistoryList has one time that is of the right type
    EXTRACT_HISTORY_ONLY(lineage, myHistory, GIHistory);

    PROFILING2_INSTANT("cha", 1, GetName());

    num_chunks_out--; // one less chunk un-acked
    num_chunks_in_flight--;

    // Remove chunk from mapping
    ChunkID cID = myHistory.get_whichChunk();
    ChunkID key;
    ChunkContainer value;
    FATALIF( ! chunkMap.IsThere( cID ), "Got an ACK for a chunk I don't know about!" );
    chunkMap.Remove( cID, key, value );

    // Are we done with everything?
    if( num_chunks_out == 0 && num_open_streams == 0 ) {
        QueryExitContainer allComplete;
        allComplete.copy( myExits );

        LOG_ENTRY_P(2, "GI %s finished processing ALL files.", GetName().c_str());
        DictionaryManager::Flush();

        SendQueryDoneMsg( allComplete );
    } else {
        RequestTokens();
    }
}
