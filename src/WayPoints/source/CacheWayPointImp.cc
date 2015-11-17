// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

#include "CacheWayPointImp.h"
#include "Logging.h"
#include "Profiling.h"
#include "WPFExitCodes.h"
#include "CPUWorkerPool.h"

using namespace std;

CacheWayPointImp :: CacheWayPointImp() :
    WayPointImp(),
    allReceived(false),
    chunksAvailable(),
    chunksOut(),
    chunksDone(),
    nextID(0),
    nChunks(0),
    nAcked(0)
{
    // Only 1 token request at a time.
    SetTokensRequested(CPUWorkToken::type, 1);
}

CacheWayPointImp :: ~CacheWayPointImp() {

}

void CacheWayPointImp :: TypeSpecificConfigure( WayPointConfigureData & configData ) {
    PDEBUG("CacheWayPointImp :: TypeSpecificConfigure()");
}

void CacheWayPointImp :: ProcessHoppingUpstreamMsg( HoppingUpstreamMsg & message ) {
    if( CHECK_DATA_TYPE( message.get_msg(), StartProducingMsg) ) {
        chunksOut.MoveToStart();
        FATALIF( ! chunksOut.AtEnd(), "Cache Waypoint restarted with chunks still unacknowledged!");
        // TODO: Maybe in the future keep track per query of the chunks to be sent.
        // We would reset that query here.
        chunksAvailable.swap(chunksDone);

        // If we have cached chunks, generate token requests to start sending them
        chunksAvailable.MoveToStart();

        // If we have previously received all chunks
        // start producing immediately
        if (allReceived)
            GenerateTokenRequests();
        else
            SendHoppingUpstreamMsg( message );

        nAcked = 0;

    } else {
        SendHoppingUpstreamMsg( message );
    }
}

void CacheWayPointImp :: ProcessHoppingDataMsg (HoppingDataMsg &data) {
    PDEBUG("CacheWayPointImp :: ProcessHoppingDataMsg()");
    if( CHECK_DATA_TYPE(data.get_data(), ChunkContainer) ) {
        GenericWorkToken returnVal;
        if( !RequestTokenImmediate(CPUWorkToken::type, returnVal) ) {
            PROFILING2_INSTANT("chd", 1, GetName());
            SendDropMsg( data.get_dest(), data.get_lineage() );
            return;
        }


        QueryExitContainer whichOnes;
        QueryExitContainer whichOnesCopy;
        whichOnes.copy(data.get_dest());
        whichOnesCopy.copy(whichOnes);

        CPUWorkToken myToken;
        myToken.swap(returnVal);

        ChunkContainer temp;
        data.get_data().swap(temp);

        // Add a new history entry
        HistoryList lineage;
        lineage.copy(data.get_lineage());

        // Immediately acknowledge chunk
        SendAckMsg(whichOnesCopy, lineage);
        nChunks++;

        lineage.Clear();
        CacheHistory history(GetID(), nextID++);
        lineage.Append(history);


        CacheChunkWD workDesc(temp);
        WorkFunc myFunc = GetWorkFunction(CacheChunkWorkFunc::type);
        WayPointID myID = GetID();

        myCPUWorkers.DoSomeWork(myID, lineage, whichOnes, myToken, workDesc, myFunc);
    } else {
        FATAL("CacheWayPointImp got a hopping data message containing a type it didn't expect");
    }
}

void CacheWayPointImp :: RequestGranted( GenericWorkToken & returnVal ) {
    PDEBUG("CacheWayPointImp :: RequestGranted()");
    CPUWorkToken myToken;
    myToken.swap(returnVal);

    Keyify<uint64_t> id;
    CachedChunk chunk;
    chunksAvailable.MoveToStart();
    chunksAvailable.Remove(chunksAvailable.CurrentKey(), id, chunk);

    QueryExitContainer whichOnes;
    whichOnes.copy(chunk.get_whichExits());

    HistoryList lineage;
    lineage.copy(chunk.get_lineage());

    CacheChunkWD workDesc(chunk.get_myChunk());
    WorkFunc myFunc = GetWorkFunction(CacheChunkWorkFunc::type);
    WayPointID myID = GetID();

    myCPUWorkers.DoSomeWork(myID, lineage, whichOnes, myToken, workDesc, myFunc);

    TokenRequestCompleted(CPUWorkToken::type);
}

void CacheWayPointImp :: DoneProducing( QueryExitContainer & whichOnes, HistoryList &lineage, int returnVal, ExecEngineData & data ) {
    PDEBUG("CacheWayPointImp :: DoneProducing()");
    // Extract data and make a copy
    ChunkContainer temp;
    temp.swap(data);
    ChunkContainer dataCopy;
    dataCopy.copy(temp);
    temp.swap(data);

    // Get history, extract ID, and make a copy
    EXTRACT_HISTORY_ONLY(lineage, history, CacheHistory);
    uint64_t id = history.get_whichChunk();
    PUTBACK_HISTORY(lineage, history);

    HistoryList lineageCopy;
    lineageCopy.copy(lineage);

    QueryExitContainer whichOnesCopy;
    whichOnesCopy.copy(whichOnes);

    // Insert copy of chunk into cache
    CachedChunk cache(dataCopy, lineageCopy, whichOnesCopy);
    Keyify<uint64_t> key(id);
    chunksOut.Insert(key, cache);

    // If we still have chunks available to produce, get more tokens.
    chunksAvailable.MoveToStart();
    if( !chunksAvailable.AtEnd() )
        GenerateTokenRequests();
}

void CacheWayPointImp :: ProcessDropMsg( QueryExitContainer & whichOnes, HistoryList & lineage ) {
    PDEBUG("CacheWayPointImp :: ProcessDropMsg()");
    PROFILING2_INSTANT("chn", 1, GetName());
    EXTRACT_HISTORY_ONLY(lineage, history, CacheHistory);
    Keyify<uint64_t> id(history.get_whichChunk());
    PUTBACK_HISTORY(lineage, history);

    FATALIF( !chunksOut.IsThere(id), "Got drop for chunk that isn't waiting for acknowledgement!");
    Keyify<uint64_t> key;
    CachedChunk chunk;
    chunksOut.Remove(id, key, chunk);

    chunksAvailable.Insert(key, chunk);

    GenerateTokenRequests();
}

void CacheWayPointImp :: ProcessAckMsg( QueryExitContainer & whichOnes, HistoryList & lineage ) {
    PDEBUG("CacheWayPointImp :: ProcessAckMsg()");
    PROFILING2_INSTANT("cha", 1, GetName());

    EXTRACT_HISTORY_ONLY(lineage, history, CacheHistory);
    Keyify<uint64_t> id(history.get_whichChunk());
    PUTBACK_HISTORY(lineage, history);

    QueryExitContainer whichOnesCopy;
    whichOnesCopy.copy(whichOnes);

    FATALIF( !chunksOut.IsThere(id), "Got drop for chunk that isn't waiting for acknowledgement!");
    Keyify<uint64_t> key;
    CachedChunk chunk;
    chunksOut.Remove(id, key, chunk);

    chunksDone.Insert(key, chunk);

    nAcked++;

    if( allReceived && (nAcked == nChunks) ) {
        SendQueryDoneMsg(whichOnesCopy);
    }
}

void CacheWayPointImp :: ProcessHoppingDownstreamMsg( HoppingDownstreamMsg &message ) {
    PDEBUG("CacheWayPointImp :: ProcessHoppingDownstreamMsg()");
    // see if we have a query done message
    if (CHECK_DATA_TYPE (message.get_msg (), QueryDoneMsg)) {
        QueryDoneMsg temp;
        temp.swap(message.get_msg());
        QueryExitContainer whichOnes;
        whichOnes.copy(temp.get_whichOnes());
        temp.swap(message.get_msg());

        allReceived = true;
        if (nAcked == nChunks) {
            SendQueryDoneMsg(whichOnes);
        }
    }
    else {
        SendHoppingDownstreamMsg (message);
    }
}

