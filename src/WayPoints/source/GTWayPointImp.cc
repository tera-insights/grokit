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

#include "GTWayPointImp.h"
#include "CPUWorkerPool.h"
#include "Logging.h"
#include "Constants.h"

#include <iostream>

using namespace std;

// Note: I have arranged the function definitions here in a different order
// than they are declared in the header. This is because I wanted to arrange
// the definitions in approximately the order the functions should be called.

GTWayPointImp :: GTWayPointImp() {
    PDEBUG( "GTWayPointImp :: GTWayPointImp()" );
    SetResultExitCode( WP_PROCESS_CHUNK );
}

GTWayPointImp :: ~GTWayPointImp() {
    PDEBUG( "GTWayPointImp :: ~GTWayPointImp()" );
}

void GTWayPointImp :: TypeSpecificConfigure( WayPointConfigureData& configData ) {
    PDEBUG( "GTWayPointImp :: TypeSpecificConfigure()" );

    GPWayPointImp::Configure( configData );

    QueryExitContainer queries;
    GetFlowThruQueryExits(queries);

    QueryIDSet newQueries;
    FOREACH_TWL(iter, queries) {
        QueryID q=iter.query;
        GLAStateContainer cont; // empty container
        if( !queryToGTs.IsThere(q) ) {
            newQueries.Union(q);
            queryToGTs.Insert(q, cont);
        }
    } END_FOREACH;
}

bool GTWayPointImp :: ReceivedStartProducingMsg( HoppingUpstreamMsg& message, QueryExit& whichOne ) {
    PDEBUG( "GTWayPointImp :: ReceivedStartProducingMsg()" );

    QueryID qID = whichOne.query;

    if( NumStatesNeeded(qID) == 0 ) {
        GotAllStates(qID);
    } else {
        StartTerminatingExits(qID);
    }

    return true;
}

bool GTWayPointImp :: PreProcessingPossible( CPUWorkToken& token ) {
    PDEBUG( "GTWayPointImp :: PreProcessingPossible()" );

    if( queriesToPreprocess.IsEmpty() )
        return false;

    QueryIDSet curQueries = queriesToPreprocess;
    // Clear out queriesToPreprocess
    queriesToPreprocess.Difference(curQueries);

    HistoryList lineage;

    QueryExitContainer qExits;

    while( !curQueries.IsEmpty() ) {
        QueryID temp = curQueries.GetFirst();

        QueryExit qe = GetExit( temp );
        qExits.Append(qe);
    }

    QueryExitContainer whichOnes;
    whichOnes.copy(qExits);

    QueryToGLASContMap reqStates;
    GetReqStates(reqStates);

    GTPreProcessWD workDesc( qExits, reqStates );

    WayPointID myID = GetID();
    WorkFunc myFunc = GetWorkFunction( GTPreProcessWorkFunc :: type );

    myCPUWorkers.DoSomeWork( myID, lineage, whichOnes, token, workDesc, myFunc );

    return true;
}

bool GTWayPointImp :: PreProcessingComplete( QueryExitContainer& whichOnes,
        HistoryList& history, ExecEngineData& data ) {
    PDEBUG( "GTWayPointImp :: PreProcessingComplete()" );

    GTPreProcessRez result;
    result.swap(data);

    QueryExitContainer endAtMe;
    GetEndingQueryExits(endAtMe);

    // Any const states that were generated.
    QueryToGLAStateMap& rezConstStates = result.get_constStates();
    AddConstStates( rezConstStates );

    QueryExitContainer startProcessing;
    QueryIDSet startTerminating;

    FOREACH_TWL( curQuery, whichOnes ) {
        queriesProcessing.Union(curQuery.query);
        SendStartProducingMsg(curQuery);
    } END_FOREACH;

    return true;
}

void GTWayPointImp :: GotAllStates( QueryID query ) {
    PDEBUG( "GTWayPointImp :: GotAllStates ()");
    // Got the last state we needed.
    queriesToPreprocess.Union(query);

    GenerateTokenRequests();
}

void GTWayPointImp :: GotChunkToProcess( CPUWorkToken& token,
        QueryExitContainer& whichOnes, ChunkContainer& chunk, HistoryList& history ) {
    PDEBUG( "GTWayPointImp :: GotChunkToProcess()" );

    // Build the work spec
    QueryToGLAStateMap qToFilter;
    FOREACH_TWL( qe, whichOnes ) {
        QueryID qID = qe.query;
        FATALIF(!queryToGTs.IsThere(qID), "Did not find a filter container for a query!");
        GLAStateContainer& cont = queryToGTs.Find(qID);
        if( cont.Length() > 0 ) {
            GLAState state;
            cont.Remove(state);
            qToFilter.Insert(qID, state);
        }
    } END_FOREACH;

    QueryExitContainer whichOnesCopy;
    whichOnesCopy.copy( whichOnes );

    QueryToGLAStateMap qToConstState;
    GetConstStates(qToConstState);

    GTProcessChunkWD workDesc( whichOnes, qToFilter, qToConstState, chunk.get_myChunk());

    WorkFunc myFunc = GetWorkFunction( GTProcessChunkWorkFunc::type );

    WayPointID myID = GetID();
    myCPUWorkers.DoSomeWork( myID, history, whichOnesCopy, token, workDesc, myFunc );
}

bool GTWayPointImp :: ProcessChunkComplete( QueryExitContainer& whichOnes,
        HistoryList& history, ExecEngineData& data ) {
    PDEBUG( "GTWayPointImp :: ProcessChunkComplete()" );

    GTProcessChunkRez result;
    result.swap(data);

    QueryToGLAStateMap& filters = result.get_filters();

    FOREACH_EM(key, filter, filters) {
        FATALIF(!queryToGTs.IsThere(key), "Got back filters for a query we don't know about!");
        GLAStateContainer& cont = queryToGTs.Find(key);
        cont.Insert(filter);
    } END_FOREACH;

    Chunk myChunk;
    Chunk& rezChunk = result.get_chunk();
    myChunk.swap(rezChunk);

    // Create the chunk container and swap it with data, so it will be sent
    // to the next waypoint
    ChunkContainer chunkCont(myChunk);
    chunkCont.swap(data);

    return false; // don't send ack
}

