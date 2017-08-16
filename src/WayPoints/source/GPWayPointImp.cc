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

#include <cstdio>

#include "GPWayPointImp.h"
#include "CPUWorkerPool.h"
#include "Logging.h"
#include "Profiling.h"

using namespace std;

GPWayPointImp :: GPWayPointImp () :
    WayPointImp(),
    queryIdentityMap(),
    constStates(),
    requiredStates(),
    statesNeeded(),
    constStateIndex(),
    resultExitCode( WP_FINALIZE )
{
    PDEBUG ("GPWayPointImp :: GPWayPointImp ()");
    SetTokensRequested( CPUWorkToken::type, NUM_EXEC_ENGINE_THREADS / 2 );
}

GPWayPointImp :: ~GPWayPointImp () {
    PDEBUG ("GPWayPointImp :: GPWayPointImp ()");
}

void GPWayPointImp :: Configure( WayPointConfigureData& configData ) {
    PDEBUG("GPWayPointImp :: TypeSpecificConfigure()");

    GPWConfigureData myConfig;
    myConfig.swap(configData);

    QueryExitContainer queries;
    GetFlowThruQueryExits(queries);

    FOREACH_TWL(iter, queries) {
        QueryID q = iter.query;
        QueryExit temp = iter;

        queryIdentityMap.Insert(q, temp);
    } END_FOREACH;

    QueryToReqStates& reqStates = myConfig.get_reqStates();
    reqStates.MoveToStart();
    FATALIF(reqStates.AtEnd(), "Why is the mapping of queries to required states empty?");

    InitReqStates( reqStates );
}

QueryExit GPWayPointImp :: GetExit( QueryID qID ) {
    PDEBUG("GPWayPointImp :: GetExit()");
    FATALIF( !queryIdentityMap.IsThere( qID ),
            "Cannot retrieve QueryExit for unknown query %s.", qID.GetStr().c_str());
    return queryIdentityMap.Find( qID );
}

void GPWayPointImp :: InitReqStates( QueryToReqStates& reqStates ) {
    PDEBUG ("GPWayPointImp :: InitReqStates ()");
    reqStates.MoveToStart();
    FATALIF(reqStates.AtEnd(), "Why is the mapping of queries to required states empty?");
    FOREACH_EM(query, list, reqStates) {
        int index = 0;
        GLAStateContainer myReqStates;
        FOREACH_TWL(sourceWP, list) {
            constStateIndex[query][sourceWP] = index++;
            GLAState dummy;
            myReqStates.Append(dummy);
        } END_FOREACH;

        QueryID key = query;
        requiredStates.Insert(key, myReqStates);

        key = query;
        Swapify<int> numNeeded(index);
        statesNeeded.Insert(key, numNeeded);

    } END_FOREACH;
}

int GPWayPointImp :: NumStatesNeeded( QueryID qID ) {
    PDEBUG ("GPWayPointImp :: NumStatesNeeded ()");
    FATALIF( !statesNeeded.IsThere( qID ), "NumStatesNeeded: Don't know about query %s.", qID.GetStr().c_str());
    return statesNeeded.Find( qID ).GetData();
}

void GPWayPointImp :: AddConstStates( QueryToGLAStateMap& states ) {
    PDEBUG ("GPWayPointImp :: AddConstStates()");
    FOREACH_EM(query, state, states) {
        FATALIF(constStates.IsThere(query), "Attempting to add duplicate constant state for query %s",
                query.GetStr().c_str());
    } END_FOREACH;

    constStates.SuckUp(states);
}

void GPWayPointImp :: GetConstStates( QueryToGLAStateMap& putHere ) {
    PDEBUG ("GPWayPointImp :: GetConstStates()");
    putHere.copy(constStates);
}

QueryToGLAStateMap GPWayPointImp :: GetConstStates() {
    return constStates;
}

QueryToGLASContMap GPWayPointImp :: GetReqStates() {
    PDEBUG ("GPWayPointImp :: GetReqStates()");
    QueryToGLASContMap reqStateCopy;
    reqStateCopy.copy( requiredStates );
    return reqStateCopy;
}

void GPWayPointImp :: GetReqStates( QueryToGLASContMap& putHere ) {
    PDEBUG ("GPWayPointImp :: GetReqStates()");
    putHere.copy(requiredStates);
}

void GPWayPointImp :: RemoveQueryData( QueryIDSet toEject ) {
    PDEBUG ("GPWayPointImp :: RemoveQueryData()");
    QueryIDSet temp = toEject;
    while( !temp.IsEmpty() ) {
        QueryID curID = temp.GetFirst();

        QueryID key;
        GLAStateContainer stateVal;
        if( requiredStates.IsThere( curID ) )
            requiredStates.Remove( curID, key, stateVal );

        GLAState tmpState;
        if( constStates.IsThere(curID) )
            constStates.Remove(curID, key, tmpState);

        Swapify<int> intVal;
        if( statesNeeded.IsThere( curID ) )
            statesNeeded.Remove( curID, key, intVal );

        constStateIndex.erase( curID );
    }
}

void GPWayPointImp :: StartTerminatingExits( QueryIDSet queries ) {
    PDEBUG ("GPWayPointImp :: StartTerminatingExits ()");

    QueryExitContainer termExits;
    GetEndingQueryExits(termExits);

    FOREACH_TWL(iter, termExits) {
        if( iter.query.Overlaps(queries) ) {
            SendStartProducingMsg( iter );
        }
    } END_FOREACH;
}

bool GPWayPointImp :: PreProcessingPossible( CPUWorkToken& token ) {
    PDEBUG ("GPWayPointImp :: PreProcessingPossible ()");
    return false;
}

bool GPWayPointImp :: PrepareRoundPossible( CPUWorkToken& token ) {
    PDEBUG ("GPWayPointImp :: PrepareRoundPossible ()");
    return false;
}

void GPWayPointImp :: GotChunkToProcess( CPUWorkToken& token, QueryExitContainer& whichOnes,
        ChunkContainer& chunk, HistoryList& lineage) {
    FATAL("GPWayPointImp doesn't know what do with a chunk!");
}

bool GPWayPointImp :: ProcessingPossible( CPUWorkToken& token ) {
    PDEBUG ("GPWayPointImp :: ProcessingPossible ()");
    return false;
}

bool GPWayPointImp :: PostProcessingPossible( CPUWorkToken& token ) {
    PDEBUG ("GPWayPointImp :: PostProcessingPossible ()");
    return false;
}

bool GPWayPointImp :: PreFinalizePossible( CPUWorkToken& token ) {
    PDEBUG ("GPWayPointImp :: PreFinalizePossible ()");
    return false;
}

bool GPWayPointImp :: FinalizePossible( CPUWorkToken& token ) {
    PDEBUG ("GPWayPointImp :: FinalizePossible ()");
    return false;
}

bool GPWayPointImp :: PostFinalizePossible( CPUWorkToken& token ) {
    PDEBUG ("GPWayPointImp :: PostFinalizePossible ()");
    return false;
}

bool GPWayPointImp :: PreProcessingComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data ) {
    PDEBUG ("GPWayPointImp :: PreProcessingComplete ()");
    return false;
}

bool GPWayPointImp :: PrepareRoundComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data ) {
    PDEBUG ("GPWayPointImp :: PrepareRoundComplete ()");
    return false;
}

bool GPWayPointImp :: ProcessChunkComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data ) {
    PDEBUG ("GPWayPointImp :: ProcessChunkComplete ()");
    return false;
}

bool GPWayPointImp :: ProcessingComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data ) {
    PDEBUG ("GPWayPointImp :: ProcessingComplete ()");
    return false;
}

bool GPWayPointImp :: PostProcessingComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data ) {
    PDEBUG ("GPWayPointImp :: PostProcessingComplete ()");
    return false;
}

bool GPWayPointImp :: PreFinalizeComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data ) {
    PDEBUG ("GPWayPointImp :: PreFinalizeComplete ()");
    return false;
}

bool GPWayPointImp :: FinalizeComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data ) {
    PDEBUG ("GPWayPointImp :: FinalizeComplete ()");
    return false;
}

bool GPWayPointImp :: PostFinalizeComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data ) {
    PDEBUG ("GPWayPointImp :: PostFinalizeComplete ()");
    return false;
}

void GPWayPointImp :: DoneProducing (QueryExitContainer &whichOnes, HistoryList &history, int result, ExecEngineData& data) {
    PDEBUG ("GPWayPointImp :: DoneProducing ()");
    // we do not touch data

    bool generateTokens = false;

    // Notify that a token request was successful if we did anything other than
    // process a chunk.
    if (result != WP_PROCESS_CHUNK ){
        TokenRequestCompleted( CPUWorkToken::type );
    }

    // Pre-Processing complete
    if( result == WP_PREPROCESSING ) {
        generateTokens = PreProcessingComplete( whichOnes, history, data );
    }

    if( result == WP_PREPARE_ROUND ) {
        generateTokens = PrepareRoundComplete( whichOnes, history, data );
    }

    // Chunk processing function complete
    if (result == WP_PROCESS_CHUNK) {
        bool res = ProcessChunkComplete( whichOnes, history, data );
        PROFILING2_INSTANT("chp", 1, GetName());
        if (res)
            SendAckMsg (whichOnes, history);
    }

    if (result == WP_PROCESSING) {
        generateTokens = ProcessingComplete( whichOnes, history, data );
    }

    // Post Processing function complete
    else if (result == WP_POST_PROCESSING){
        generateTokens = PostProcessingComplete( whichOnes, history, data );
    }

    // Pre Finalize function complete
    else if (result == WP_PRE_FINALIZE) {
        generateTokens = PreFinalizeComplete( whichOnes, history, data );
    }

    // Finalize function complete
    else if (result == WP_FINALIZE ) {
        generateTokens = FinalizeComplete( whichOnes, history, data );
    }

    // Post Finalize function complete
    else if (result == WP_POST_FINALIZE ) {
        generateTokens = PostFinalizeComplete( whichOnes, history, data );
    }

    if( generateTokens )
        GenerateTokenRequests();

    if (result != resultExitCode){ // not finalize, kill the output since nothing gets to the top
        // zero-out data so the EE does not send it above
        ExecEngineData dummy;
        data.swap(dummy);
    }
}

void GPWayPointImp :: RequestGranted (GenericWorkToken &returnVal) {
    PDEBUG ("GPWayPointImp :: RequestGranted ()");

    // we know that the reason that this request is granted is that we have one or more
    // query exits that we are ready to finish up... first, cast the return val appropriately
    CPUWorkToken myToken;
    myToken.swap (returnVal);

    if( PostFinalizePossible( myToken ) ) {
        return;
    }
    else if( FinalizePossible( myToken ) ) {
        return;
    }
    else if( PreFinalizePossible( myToken ) ) {
        return;
    }
    else if( PostProcessingPossible( myToken ) ) {
        return;
    }
    else if ( ProcessingPossible( myToken ) ) {
        return;
    }
    else if ( PrepareRoundPossible( myToken ) ) {
        return;
    }
    else if( PreProcessingPossible( myToken ) ) {
        return;
    }
    else {// nothing to do
        TokenRequestCompleted( CPUWorkToken::type );
        GiveBackToken(myToken);
    }
}

void GPWayPointImp :: ProcessHoppingDataMsg (HoppingDataMsg &data) {
    PDEBUG ("GPWayPointImp :: ProcessHoppingDataMsg ()");

    if( CHECK_DATA_TYPE(data.get_data(), ChunkContainer) ) {
        // in this case, the first thing we do is to request a work token
        GenericWorkToken returnVal;
        if (!RequestTokenImmediate (CPUWorkToken::type, returnVal)) {

            // if we do not get one, then we will just return a drop message to the sender
            PROFILING2_INSTANT("chd", 1, GetName());
            SendDropMsg (data.get_dest (), data.get_lineage ());
            return;
        }

        // convert the token into the correct type
        CPUWorkToken myToken;
        myToken.swap (returnVal);

        // OK, got a token!  So first thing is to extract the chunk from the message
        ChunkContainer temp;
        data.get_data ().swap (temp);

        // at this point, we are ready to create the work spec.  First we figure out what queries to finish up
        QueryExitContainer whichOnes;
        whichOnes.copy (data.get_dest ());

        // if we have a chunk produced by a table waypoint log it
        CHECK_FROM_TABLE_AND_LOG( data.get_lineage(), GLAWayPoint );

        GotChunkToProcess( myToken, whichOnes, temp, data.get_lineage() );
    }
    else if( CHECK_DATA_TYPE(data.get_data(), StateContainer) ) {
        StateContainer temp;
        data.get_data().swap( temp );

        GotState( temp );
    }
    else {
        FATAL("GLAWaypoint got a hopping data message containing a type of data it "
                "didn't expect!");
    }
}

// the only kind of message we are interested in is a query done message... everything else is
// just forwarded on, down the graph
void GPWayPointImp:: ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message) {
    PDEBUG ("GPWayPointImp :: ProcessHoppingDownstreamMsg ()");

    bool retVal = false;

    // see if we have a query done message
    if (CHECK_DATA_TYPE (message.get_msg (), QueryDoneMsg)) {

        // do the cast via a swap
        QueryDoneMsg temp;
        temp.swap (message.get_msg ());

        retVal = ReceivedQueryDoneMsg( temp.get_whichOnes() );
    }
    else {
        SendHoppingDownstreamMsg (message);
    }

    if( retVal ) {
        GenerateTokenRequests();
    }
}

void GPWayPointImp :: ProcessHoppingUpstreamMsg( HoppingUpstreamMsg& message) {
    PDEBUG("GPWayPointImp :: ProessHoppingUpstreamMsg()");
    bool retVal = false;

    if( CHECK_DATA_TYPE( message.get_msg(), StartProducingMsg) ) {
        StartProducingMsg temp;
        temp.swap( message.get_msg() );
        QueryExit whichOne = temp.get_whichOne();
        temp.swap( message.get_msg() );

        retVal = ReceivedStartProducingMsg( message, whichOne );
    }
    else {
        SendHoppingUpstreamMsg( message );
    }

    if( retVal ) {
        GenerateTokenRequests();
    }
}

bool GPWayPointImp :: ReceivedStartProducingMsg(HoppingUpstreamMsg& message, QueryExit& whichOne ) {
    // Default behaior: just forward it.
    SendHoppingUpstreamMsg(message);
    return false;
}

void GPWayPointImp :: GotState( StateContainer& state ) {
    PDEBUG("GPWayPointImp :: GotState()");
    // Extract information from the state container.

    QueryExit qe;
    state.get_whichQuery().swap(qe);

    WayPointID source = state.get_source();

    // DEBUG
    //fprintf("[%s] Got State From %s\n", GetID().getName().c_str(), source.getName().c_str());
    // END DEBUG

    GLAState myState;
    state.get_myState().swap(myState);

    // Get information we have about the query.
    FATALIF( !statesNeeded.IsThere( qe.query ), "Got a state container for a query we don't know about!");
    Swapify<int>& tempStatesNeeded = statesNeeded.Find(qe.query);
    int myStatesNeeded = tempStatesNeeded.GetData();

    FATALIF( myStatesNeeded == 0, "Got a state for a query that doesn't need any more states!" );

    FATALIF( !requiredStates.IsThere( qe.query ), "Got a state container for a query we have no const states for!");
    GLAStateContainer& myReqStates = requiredStates.Find(qe.query);

    myReqStates.MoveToStart();

    int whichIndex = constStateIndex[qe.query][source];

    for( int i = 0; i < whichIndex; ++i ) {
        myReqStates.Advance();
    }

    GLAState& curState = myReqStates.Current();
    myState.swap(curState);

    // Update the number of states needed
    --myStatesNeeded;
    Swapify<int> tempVal(myStatesNeeded);
    tempStatesNeeded.swap(tempVal);

    if( myStatesNeeded == 0 ) {
        GotAllStates( qe.query );
    }
}

void GPWayPointImp :: SetResultExitCode( ExitCode exitCode ) {
    resultExitCode = exitCode;
}

bool GPWayPointImp :: ReceivedQueryDoneMsg( QueryExitContainer& whichOnes ) {
    PDEBUG( "GPWayPointImp :: ReceivedQueryDoneMsg()" );

    // Just forward it
    SendQueryDoneMsg( whichOnes );

    return false;
}
