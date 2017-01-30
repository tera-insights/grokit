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
#include "TableWayPointImp.h"
#include "Constants.h"
#include "DiskPool.h"
#include "QueryManager.h"
#include "Logging.h"
#include "Profiling.h"
#include "CPUWorkerPool.h"
#include "RateLimiter.h"

#include <cmath>

using namespace std;

extern EventProcessor globalCoordinator;


void TableWayPointImp::Debugg(void){
    printf("\n Table scanner %s has %d requests out", myName.c_str(), numRequestsOut);
    queryChunkMap->Debugg();
}

Bitstring TableWayPointImp::FindQueries(off_t _chunkId){
    //int qwbSize; // will contain the number of rows in the bitmap≈

    //return queryChunkMap->GetBits(qwbSize)[_chunkId];
    return queryChunkMap->GetBits(_chunkId);
}

TableWayPointImp :: TableWayPointImp () :
    WayPointImp(),
    fileId(),
    myName(),
    queryChunkMap(nullptr),
    ackQueries(nullptr),
    qeTranslator(),
    colManager(),
    qeCounters(),
    doneQueries(),
    numRequestsOut(0),
    lastChunkId(0),
    numChunks(0),
    clusterRanges(),
    queryClusterRanges(),
    limiter()
{
    PDEBUG ("TableWayPointImp :: TableWayPointImp ()");
}

TableWayPointImp :: ~TableWayPointImp () {
    PDEBUG ("~TableWayPointImp :: TableWayPointImp ()");
}

void TableWayPointImp :: TypeSpecificConfigure (WayPointConfigureData &configData) {
    PDEBUG ("TableWayPointImp :: TypeSpecificConfigure ()");

    // first, extract the extra config info
    TableConfigureData tempConfig;
    tempConfig.swap (configData);

    // check if the file scanner is started, if not contact the diskPool and start it
    // make sure the internal datastructures get initialized at this time
    if (!fileId.IsValid()){
        // start file

        // need to determine the number of columns
        // we do it the slow way: count the number from the map
        int _numCols = 0;
        FOREACH_EM(key, data, tempConfig.get_columnsToSlotsMap()){
            _numCols++;
        }END_FOREACH;

        fileId = globalDiskPool.AddFile(tempConfig.get_relName(), _numCols);
        myName = tempConfig.get_relName();

        numChunks = globalDiskPool.NumChunks(fileId);
        clusterRanges = globalDiskPool.ClusterRanges(fileId);

        PDEBUG("Relation %s has %d chunks", myName.c_str(), numChunks);
        queryChunkMap =  new QueryChunkMap(numChunks);
        ackQueries = new QueryChunkMap(numChunks);
    }

    // Update cluster ranges for each new query
    QueryToScannerRangeList& newRanges = tempConfig.get_filterRanges();
    for(auto elem : newRanges) {
        queryClusterRanges[elem.first] = elem.second;
    }

    // delete queryExits first from the query termination trackers
    // NOTE: we delete before adding since the queryExits could have been recycled
    QueryExitContainer& delQueries = tempConfig.get_deletedQE();
    for (delQueries.MoveToStart(); !delQueries.AtEnd(); delQueries.Advance()){
        qeCounters.erase(delQueries.Current());
    }


    // go over all the new queryExits and reset the counters for query termination
    QueryExitContainer& queries = tempConfig.get_newQE();
    for (queries.MoveToStart(); !queries.AtEnd(); queries.Advance()){
        QueryExit qe = queries.Current();
        FATALIF(qe.query.IsEmpty(), "Relation %s got a query exit with an empty query");

        WARNINGIF( (qeCounters.find(qe) != qeCounters.end()), "We are asked to add a query that we already know about");

        // NOTE: if we allow appends while we run, we have to be careful here with reseting
        // the counter. For now, it is simply the number of chunks
        qeCounters[qe] = numChunks;
    }

    // tell the translator its part of the config
    Bitstring dummy = qeTranslator.queryExitToBitstring(queries, true);
    qeTranslator.deleteQueryExits(tempConfig.get_deletedQE());

    // tell the column manager its part of the config
    colManager.ChangeMapping(tempConfig.get_queryColumnsMap(),
            tempConfig.get_columnsToSlotsMap(),
            tempConfig.get_storeColumnsToSlotsMap(),
            delQueries);

    // tell people that we are ready to go with our queries... these messages will
    // eventually make it down to the table scan, which will beging producing data
    QueryExitContainer endingOnes;
    GetEndingQueryExits (endingOnes);
    for (endingOnes.MoveToStart (); endingOnes.RightLength (); endingOnes.Advance ()) {

        // get the meta data
        QueryExit tempExit = endingOnes.Current (), tempExitCopy = endingOnes.Current ();
        WayPointID myID = GetID (), myIDCopy = GetID ();

        // create the actual notification fiest
        StartProducingMsg startProducing (myID, tempExit);

        // now wrap it up in a hopping upstream message
        HoppingUpstreamMsg myMessage (myIDCopy, tempExitCopy, startProducing);
        SendHoppingUpstreamMsg (myMessage);
    }
}

void TableWayPointImp::GenerateTokenRequests(){
    PDEBUG ("TableWayPointImp :: GenerateTokenRequests()");
    std::printf("%s has average ack time %llu\n", myName.c_str(), 
    	limiter.GetAverageAckTime());
    for (; numRequestsOut < FILE_SCANNER_MAX_NO_CHUNKS_REQUEST; numRequestsOut++) {
      RequestTokenDelayOK (DiskWorkToken::type, limiter.GetMinStart());
    }
}

void TableWayPointImp :: RequestGranted (GenericWorkToken &returnVal) {
    PDEBUG ("TableWayPointImp :: RequestGranted()");

    // the reason that this request was granted is that we had previously asked for a token that would
    // allow us to go off and produce a new chunk.  Note that in this toy version of the TableWayPointImp
    // the table scanner actually uses a CPU worker to produce its chunks, and it actually sends a function
    // off to a CPU to get the chunk produced.  In a "real" version of this waypoint, we would presumably
    // be requesting a disk token, and once we had it we would be asking a diferent resource (not the CPU
    // manager) to produce the chunk
    DiskWorkToken myToken;
    myToken.swap (returnVal);

    bool sentRequest = false;

    off_t _chunkId;
    while( !sentRequest && ChunkRequestIsPossible(_chunkId)) {
        //chunkId of the chunk that is going to be requested was set in the ChunkRequestIsPossible method

        // the mask of the queries for which we generate the chunk
        Bitstring queries = FindQueries(_chunkId);
        Bitstring bitIter;
        Bitstring filteredOut;

        Bitstring ackedQ = ackQueries->GetBits(_chunkId);
        ackedQ.Intersect(queries);
        WARNINGIF(!ackedQ.IsEmpty(),
                "Queries %s already produced for chunk %d of %s. Adjusting",
                ackedQ.GetStr().c_str(), _chunkId, myName.c_str());

        queries.Difference(ackedQ);

        WARNINGIF(queries.Overlaps(doneQueries),"Why am I still seeing queries that are done?");
        queries.Difference(doneQueries);

        // Filter based on clustering
        const ClusterRange& chunkRange = clusterRanges[_chunkId];
        int64_t cMin = chunkRange.first;
        int64_t cMax = chunkRange.second;
        bitIter = queries;
        while( !bitIter.IsEmpty() ) {
            QueryID qid = bitIter.GetFirst();
            ClusterRangeList& qRanges = queryClusterRanges[qid];

            bool keep = qRanges.empty() || cMin > cMax;

            for( auto iter = qRanges.begin(); !keep && iter != qRanges.end(); iter++) {
                int64_t min = iter->first;
                int64_t max = iter->second;

                /* This is a concise way of determining whether the intervals
                 * [min, max] and [cMin, cMax] intersect.
                 *
                 * Notice that if either interval is inverted, the intersection
                 * will also be inverted.
                 */
                keep = std::min(max, cMax) >= std::max(min, cMin);
            }

            if( !keep ) {
                filteredOut.Union(qid);
                queries.Difference(qid);
            }
        }

        //reset the bitmap for the requested chunk
        queryChunkMap->Clear(_chunkId);

        // Acknowledge queries in filteredOut
        if( !filteredOut.IsEmpty() ) {
            AcknowledgeChunk(_chunkId, filteredOut);
        }

        // If we have no queries need the chunk, try to find another chunk
        if( queries.IsEmpty() ) {
            continue;
        }

        QueryExitContainer myOutputExits;
        qeTranslator.bitstringToQueryExitContaiener(queries, myOutputExits);

        //get the union of the columns used by the active queries
        SlotPairContainer colsToRead;
        colManager.UnionColumns(myOutputExits, colsToRead);

        // get the lineage ready; we'll get this back when teh chunks is build
        QueryExitContainer myOutputExitsCopy;
        myOutputExitsCopy.copy (myOutputExits);
        ChunkID chunkId(_chunkId, fileId);
        TableReadHistory myHistory (GetID (), chunkId, myOutputExits);
        HistoryList lineage;
        lineage.Insert (myHistory);


        int numAvailableCPUs = myCPUWorkers.NumAvailable();
        // use uncompressed if less than USE_UNCOMPRESSED_THRESHOLD
        // fraction of threads available
        // bool useUncompressed = (numAvailableCPUs < USE_UNCOMPRESSED_THRESHOLD*NUM_EXEC_ENGINE_THREADS);
        bool useUncompressed = true;

        // send the request
        WayPointID tempID = GetID ();
        ChunkID chunkID(_chunkId, fileId);
        globalDiskPool.ReadRequest(chunkID, tempID, useUncompressed, lineage, myOutputExitsCopy, myToken, colsToRead);
        sentRequest = true;

        LOG_ENTRY_P(2, "CHUNK %d of %s REQUESTED for queries %s",
                _chunkId, myName.c_str(), queries.GetStr().c_str()) ;

    }

    if( !sentRequest) {
        // if three are no query exits that need data, then just give back the token and get out
        numRequestsOut--;
        GiveBackToken (myToken);
        return;
    }

    limiter.ChunkOut(_chunkId);

    GenerateTokenRequests();
}

void TableWayPointImp :: ProcessHoppingUpstreamMsg (HoppingUpstreamMsg &message) {
    PDEBUG ("TableWayPointImp :: ProcessHoppingUpstreamMsg()");

    Notification& msg = message.get_msg ();

    // expect multiple message types. Figure out which one this is
    if (msg.Type() == StartProducingMsg::type) {
        StartProducingMsg myMessage;
        msg.swap (myMessage);

        // NOTE: for now Chris is sending us a query at the time the code
        // to deal with multiple queries is in place. We artificially
        // place the one query in a list. Hopefully we convince him to send
        // us larger requests.

        // get the queries to start and tell our datastructure to mark them as ready
        QueryExit &queryToStart = myMessage.get_whichOne ();
        QueryExitContainer queries;
        QueryID qID = queryToStart.query;
        QueryExit qExit = queryToStart;
        queries.Insert(queryToStart);

        TableScanInfo infoTS;
        fileId.getInfo(infoTS);
        QueryManager& qm = QueryManager::GetQueryManager();
        string qName;

        // log info for scanning on behalf of queries
        for (queries.MoveToStart(); !queries.AtEnd(); queries.Advance()){
            QueryExit qe = queries.Current();
            qm.GetQueryName(qe.query, qName);
            IDInfo info;
            qe.exit.getInfo(info);

            LOG_ENTRY_P(2, "SCANNER(%s) starting (%s,%s)", infoTS.getName().c_str(),
                    qName.c_str(), info.getName().c_str());
        }

        // make sure that the queries we are supposed to start are a subset of
        // the queries we are watching
        Bitstring newQ=qeTranslator.queryExitToBitstring(queries);

        // OR this query set with all the bitmaps of the chunks that will be served
        // to this query

        // WARNING: for now we assume that we do not have bulkloads so
        // the query gets all the chunks in the system. When bulkloads are added
        // we need an object to tell us what chunks we should serve to the query
        queryChunkMap->ORAll(newQ);

        // Clear any acks we have nave gotten for this query, as we may have
        // been asked to restart for this query.
        ackQueries->DiffAll(qID);

        // Remove this query from doneQueries if it is there.
        doneQueries.Difference(qID);

        // Reset the qeCounter for this query.
        QECounters::iterator it = qeCounters.find(qExit);
        FATALIF(it == qeCounters.end(), "Found a query exit that wasn't ours in the counters.");

        int& counter = it->second;
        counter = numChunks;

        if( numChunks == 0 ) {
            // Special case for if there are no chunks in the relation.
            // Immediately send a QueryDone message.
            QueryExitContainer allCompleteCopy;
            allCompleteCopy.copy (queries);
            QueryDoneMsg someAreDone (GetID (), queries);
            HoppingDownstreamMsg myOutMsg (GetID (), allCompleteCopy, someAreDone);
            SendHoppingDownstreamMsg (myOutMsg);
        }
    }
    // this is where new messages go, such as write chunk
    else {
        FATAL( 	"Strange, why did a table scan get a HUS of a type that was not 'Start Producing'?");
    }

    // make sure we get to do some of the work we need
    GenerateTokenRequests();
}

void TableWayPointImp :: ProcessDropMsg (QueryExitContainer &whichExits,
        HistoryList &lineage) {

    PDEBUG ("TableWayPointImp :: ProcessDropMsg()");

    //printf("X"); fflush(stdout);
    PROFILING2_INSTANT("chn", 1, myName);

    // make sure that the HistoryList has one item that is of the right type
    lineage.MoveToStart ();
    FATALIF (lineage.Length () != 1 ||
            (lineage.Current ().Type()!= TableReadHistory::type),
            "Got a bad lineage item in an ack to a table scan waypoint!");

    // get the history out
    TableReadHistory myHistory;
    lineage.Remove (myHistory);

    Bitstring toKill = qeTranslator.queryExitToBitstring(whichExits);
    ChunkID cnkID = myHistory.get_whichChunk ();
    limiter.ChunkDropped(cnkID.GetID());

    Bitstring ackedQ = ackQueries->GetBits(cnkID.GetInt());
    ackedQ.Intersect(toKill);
    WARNINGIF(!ackedQ.IsEmpty(),
            "Queries %s already produced for chunk %d. Adjusting",
            ackedQ.GetStr().c_str(), cnkID.GetInt());

    toKill.Difference(ackedQ);

    WARNINGIF(toKill.Overlaps(doneQueries), "Got a chunk killed from a finished query. Chunk# %d, Queries %s", cnkID.GetInt(), toKill.GetStr().c_str());
    toKill.Difference(doneQueries);

    if (toKill.IsEmpty()){
        GenerateTokenRequests();
        return;
    }

    if ( cnkID.IsAll() ){
        // we have to kill all the chunks
        queryChunkMap->ORAll(toKill);
        WARNING("We got a kill all");
    } else {
        // we kill a specific chunk
        int chunkNo = cnkID.GetInt();
        Bitstring old=queryChunkMap->GetBits(chunkNo);

        if (old.Overlaps(toKill)){
            // deep diagnosis
            // go through all the chunks and print any discrepancy between to-produced and processed
            for (int chk=0; chk<numChunks; chk++){
                Bitstring toProd = queryChunkMap->GetBits(chk);
                Bitstring proc = ackQueries->GetBits(chk);
                if (toProd.Overlaps(proc))
                    printf("Diag CHUNK %d toProd=%s proc=%s\n",chk,
                            toProd.GetStr().c_str(), proc.GetStr().c_str());
            }
            FATAL("Stopped");
        }

        // there should be no overlap between old and new
        WARNINGIF(old.Overlaps(toKill), "Adding queries to %s already in at position %d: old %s new %s\n", myName.c_str(), chunkNo, old.GetStr().c_str(), toKill.GetStr().c_str());

        queryChunkMap->OROne(chunkNo, toKill);
    }

    LOG_ENTRY_P(2, "CHUNK %d of %s DROPPED : Queries %s",
            cnkID.GetInt(), myName.c_str(), toKill.GetStr().c_str()) ;

    GenerateTokenRequests();
}

/**
  The implementation of the method is very simplistic at this moment.
  If the size of the request pool is less than the maximum number of allowed chunks, a new request is possible.
  In the future this should be like a control system that regulates the amount of chunks that are produced.
  Profiling information is required for this more complicated behavior.
  */
bool TableWayPointImp::ChunkRequestIsPossible(off_t &_chunkId) {
    lastChunkId = queryChunkMap->FindFirstSet(lastChunkId);
    if (lastChunkId == -1){
        lastChunkId = 0; // start from the beginning next time
        return false; // we did not find any chunk
    } else {
        _chunkId = lastChunkId;
        return true;
    }
}

void TableWayPointImp::AcknowledgeChunk(int chunkID, QueryIDSet queries) {
    QueryExitContainer exits;
    PCounterList progressCounters;
    QueryManager& qm = QueryManager::GetQueryManager();
    QueryExitContainer allComplete;


    ackQueries->OROne(chunkID, queries);
    qeTranslator.bitstringToQueryExitContainer(queries, exits);

    FOREACH_TWL(qe, exits) {
        QueryID q = qe.query;
        QECounters::iterator it = qeCounters.find(qe);

        FATALIF( (it == qeCounters.end()), "Did not find the query exit. Not ours");

        int& counter = it->second;
        counter--; // count backwards

        if (counter == 0) { // we are done with this query
            allComplete.Insert (qe);
        }

        // Progress in tenths of a percent
        int64_t progress = lround(((1.0 * (numChunks - counter)) / numChunks) * 1000.0);
        std::string  qName;
        qm.GetQueryName(q, qName);
        PCounter tmp(qName, progress, myName);
        progressCounters.Append(tmp);

    } END_FOREACH;

    if( progressCounters.Length() > 0 ) {
        PROFILING2_PROGRESS_SET(progressCounters, myName);
    }

    // if anyone is done, send the notification
    if (!allComplete.IsEmpty()) {
        Bitstring newFinished = qeTranslator.queryExitToBitstring(allComplete);

        FATALIF(doneQueries.Overlaps(newFinished), "Why am I seing queries finishing multiple times");
        doneQueries.Union(newFinished); // uptate finished queries

        LOG_ENTRY_P(2, "Scanning %s for query %s FINISHED",
                myName.c_str(), newFinished.GetStr().c_str());


        QueryExitContainer allCompleteCopy;
        allCompleteCopy.copy (allComplete);
        QueryDoneMsg someAreDone (GetID (), allComplete);
        HoppingDownstreamMsg myOutMsg (GetID (), allCompleteCopy, someAreDone);
        SendHoppingDownstreamMsg (myOutMsg);

    }
}

void TableWayPointImp :: ProcessAckMsg (QueryExitContainer &whichExits, HistoryList &lineage) {
    PDEBUG ("TableWayPointImp :: ProcessAckMsg()");

    //printf("."); fflush(stdout);
    PROFILING2_INSTANT("cha", 1, myName);

    // make sure that the HistoryList has one item that is of the right type
    lineage.MoveToStart ();
    FATALIF (lineage.RightLength () != 1 || (lineage.Current ().Type() != TableReadHistory::type),
            "Got a bad lineage item in an ack to a table scan waypoint!");

    // get the history out
    TableReadHistory myHistory;
    lineage.Remove (myHistory);
    ChunkID cnkID = myHistory.get_whichChunk ();
    limiter.ChunkAcked(cnkID.GetID());

    // this is the set of totally completed queries

    FATALIF( whichExits.Length()==0, "You sent me a chunk without any query exits.");

    Bitstring toAck = qeTranslator.queryExitToBitstring(whichExits);

    AcknowledgeChunk(cnkID.GetInt(), toAck);

    LOG_ENTRY_P(2, "CHUNK %d of %s PROCESSED for queries %s",
            cnkID.GetInt(), myName.c_str(), toAck.GetStr().c_str()) ;

    GenerateTokenRequests();
}

void TableWayPointImp::ProcessHoppingDataMsg (HoppingDataMsg &data){
    PDEBUG ("TableWayPointImp :: ProcessHoppingDataMsg()");
    // this message is comming from somebody asking us to write chunks

    GenericWorkToken returnVal;
    if (!RequestTokenImmediate (DiskWorkToken::type, returnVal)) {
        // if we do not get one, then we will just return a drop message to the sender
        SendDropMsg (data.get_dest (), data.get_lineage ());
        return;
    }

    // convert the token into the correct type
    DiskWorkToken myToken;
    myToken.swap (returnVal);

    off_t _chunkId = numChunks;
    numChunks++;

    // extract the chunk from the message
    ChunkContainer chunkCnt;
    data.get_data ().swap (chunkCnt);

    // create the work spec and get it done!
    QueryExitContainer myDestinations;
    myDestinations.copy (data.get_dest ());

    // send the request
    WayPointID tempID = GetID ();
    ChunkID chunkID(_chunkId, fileId);
    SlotPairContainer colsToProcess;
    colManager.GetColsToWrite(colsToProcess);
    globalDiskPool.WriteRequest(chunkID, tempID, chunkCnt.get_myChunk(), data.get_lineage(),
            myDestinations, myToken, colsToProcess);

    LOG_ENTRY_P(2, "CHUNK %d of %s sent for WRITTING",
            _chunkId, myName.c_str()) ;

}


void TableWayPointImp::DoneProducing (QueryExitContainer &whichOnes, HistoryList &history,
        int result, ExecEngineData& data) {
    PDEBUG ("TableWayPointImp::DoneProducing()");

    // Two possibilities:
    // 1. We wrote something and this is the confirmation. Send the ack
    //    to whoever sent us the chunk
    // 2. We read something. The history is ours and  we do nothing
    // (system correctly pushes the chunk to the next guy)
    // To tell them apart we peak at the id in the history.

    bool isOurChunk = false;

    // if this is a read ack than the first element should be a TableReadHistory
    // created by us
    history.MoveToStart();
    if (CHECK_DATA_TYPE(history.Current(), TableReadHistory)) {
        TableReadHistory readHist;
        readHist.swap(history.Current());
        WayPointID srcWP = readHist.get_whichWayPoint();
        readHist.swap(history.Current());

        isOurChunk = GetID() == srcWP;

        if (isOurChunk) {
            FATALIF(history.RightLength () != 1, "Why do we have more than the table lineage?");
        }
    }

    if (isOurChunk){

        ChunkContainer cCont;
        cCont.swap(data);
        cCont.get_myChunk().MakeReadonly();
        cCont.swap(data);

        LOG_ENTRY_P(2, "CHUNK of %s READ", myName.c_str()) ;
        // ignore the message
    } else {
        // this is a write reply, send the ack to whoever
        SendAckMsg (whichOnes, history);
        // wipe out data so we get no warning
        ExecEngineData dummy;
        dummy.swap(data);
    }

    // note that we have one less request out
    numRequestsOut--;
    GenerateTokenRequests();
}

void TableWayPointImp :: ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message) {
    PDEBUG ("TableWayPointImp :: ProcessHoppingDownstreamMsg()");
    // Let's flush all metadata first before showdown
    globalDiskPool.Flush(fileId);

    // see if we have a query done message
    if (CHECK_DATA_TYPE (message.get_msg (), QueryDoneMsg)) {

        // do the cast via a swap
        QueryDoneMsg temp;
        temp.swap (message.get_msg ());

        // Find the queries terminating here and send a message to the
        // global coordinator letting him know
        // we assume that all query exits with out waypoint id end here
        // TODO: get paranoid and check this with the myExits list
        QueryExitContainer endingOnes;
        FOREACH_TWL(qe, temp.get_whichOnes ()){
            if (qe.exit == GetID()){
                QueryExit tmp = qe;
                endingOnes.Append(qe);
            }
        }END_FOREACH;

        QueriesDoneMessage_Factory(globalCoordinator, endingOnes);

    } else {
        FATAL ("Why did I get some hopping downstream message that was not a query done message?\n");
    }
}


