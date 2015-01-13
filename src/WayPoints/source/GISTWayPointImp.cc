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

#include "GISTWayPointImp.h"
#include "CPUWorkerPool.h"
#include "Logging.h"
#include "Constants.h"

#include <iostream>

using namespace std;

/***** Helper Methods *****/

int GISTWayPointImp :: DecrementInt( QueryIDToInt& jobCount, QueryID query ) {
    PDEBUG("GISTWayPointImp :: DecrementInt ()");

    FATALIF( !jobCount.IsThere( query ),
            "Tried to decrement a non-existent job count for query %s!",
            query.GetStr().c_str());

    Swapify<int>& swapCount = jobCount.Find( query );
    int& count = swapCount.GetData();

    return --count;
}

int GISTWayPointImp :: IncrementInt( QueryIDToInt& jobCount, QueryID query ) {
    PDEBUG("GISTWayPointImp :: IncrementInt ()");

    if( !jobCount.IsThere( query ) ) {
        Swapify<int> newCount(0);
        QueryID key = query;
        jobCount.Insert( key, newCount );
    }

    Swapify<int>& swapCount = jobCount.Find( query );
    int& count = swapCount.GetData();

    return ++count;
}

void GISTWayPointImp ::FinishQuery( QueryID query ) {
    PDEBUG("GISTWayPointImp :: FinishQuery (%s)", query.GetStr().c_str());

    queriesCompleted.Union(query);

    // Clean up data structures
    if( workUnits.IsThere( query ) ) {
        QueryID key;
        GistWUContainer value;
        workUnits.Remove( query, key, value );

        FATALIF(!value.IsEmpty(),
                "Finished Query %s with work left to be done!",
                query.GetStr().c_str());
    }

    if( processingJobsOut.IsThere( query ) ) {
        QueryID key;
        Swapify<int> value;
        processingJobsOut.Remove(query, key, value);

        int jobs = value.GetData();
        FATALIF(jobs != 0,
                "Finished query %s with %d processing jobs running!",
                query.GetStr().c_str(), jobs);
    }

    if( glasToMerge.IsThere(query) ) {
        QueryID key;
        GLAStateContainer value;
        glasToMerge.Remove( query, key, value );

        FATALIF(!value.IsEmpty(),
                "Finished query %s with states still to be merged!",
                query.GetStr().c_str());
    }

    FATALIF(mergedGLAs.IsThere(query),
            "Finished query %s with unused merged GLA!",
            query.GetStr().c_str());

    if( fragmentsLeft.IsThere(query) ) {
        QueryID key;
        Swapify<int> value;
        fragmentsLeft.Remove(query, key, value);

        int frags = value.GetData();

        FATALIF(frags !=  0,
                "Finished query %s with %d fragments left to produce!",
                query.GetStr().c_str(), frags);
    }

    if( query.Overlaps( queriesToIterate ) ) {
        RestartQuery(query);
    }
    else {
        QueryExitContainer qeCont;
        QueryExit qe = GetExit( query );
        qeCont.Insert(qe);

        SendQueryDoneMsg(qeCont);
    }
}

void GISTWayPointImp :: RestartQuery( QueryID query ) {
    PDEBUG("GISTWayPointImp :: RestartQuery (%s)", query.GetStr().c_str());

    FATALIF(!query.IsSubsetOf(queriesCompleted),
            "Attempted to restart incomplete query %s",
            query.GetStr().c_str());

    queriesCompleted.Difference(query);
    queriesToIterate.Difference(query);

    queriesToPrepare.Union(query);

    GenerateTokenRequests();
}

/***** End Helper Methods *****/

GISTWayPointImp :: GISTWayPointImp ()
    : lastFragmentID(0)
{
    PDEBUG("GISTWayPointImp :: GISTWayPointImp ()");
    SetTokensRequested( CPUWorkToken::type, NUM_EXEC_ENGINE_THREADS );
}

GISTWayPointImp :: ~GISTWayPointImp () {
    PDEBUG("GISTWayPointImp :: ~GISTWayPointImp ()");
}

void GISTWayPointImp :: TypeSpecificConfigure( WayPointConfigureData &configData ) {
    PDEBUG("GISTWayPointImp :: TypeSpecificConfigure ()");

    GISTConfigureData tempConfig;
    tempConfig.swap(configData);

    QueryIDToBool& retStates = tempConfig.get_resultIsState();
    resultIsState.SuckUp(retStates);

    tempConfig.swap(configData);
    GPWayPointImp::Configure(configData);

}

void GISTWayPointImp :: GotAllStates( QueryID query ) {
    PDEBUG("GISTWayPointImp :: GotAllStates (%s)", query.GetStr().c_str());

    // We can start preprocessing this query.
    queriesToPreprocess.Union(query);

    GenerateTokenRequests();
}

bool GISTWayPointImp :: PreProcessingPossible( CPUWorkToken& token ) {
    PDEBUG("GISTWayPointImp :: PreProcessingPossible ()");
    if( queriesToPreprocess.IsEmpty() )
        return false;

    QueryIDSet curQueries = queriesToPreprocess;
    queriesToPreprocess.Difference(curQueries);
    queriesPreprocessing.Union(curQueries);

    // Prepare the work description.
    HistoryList lineage;
    GISTHistory hist(GetID(), -1);
    lineage.Insert(hist);

    QueryExitContainer qExits;

    while( !curQueries.IsEmpty() ) {
        QueryID temp = curQueries.GetFirst();
        QueryExit qe = GetExit(temp);
        qExits.Append(qe);
    }

    QueryExitContainer whichOnes;
    whichOnes.copy(qExits);

    QueryToGLASContMap reqStates;
    GetReqStates( reqStates );

    GISTPreProcessWD workDesc( qExits, reqStates );

    WayPointID myID = GetID();
    WorkFunc myFunc = GetWorkFunction( GISTPreProcessWorkFunc :: type );

    myCPUWorkers.DoSomeWork( myID, lineage, whichOnes, token, workDesc, myFunc );

    return true;
}

bool GISTWayPointImp :: PreProcessingComplete( QueryExitContainer& whichOnes,
        HistoryList& history, ExecEngineData& data) {
    PDEBUG("GISTWayPointImp :: PreProcessingComplete()");

    GISTPreProcessRez result;
    result.swap(data);

    QueryToGLAStateMap& generatedStates = result.get_genStates();
    QueryToGLAStateMap& gists = result.get_gists();

    gistStates.SuckUp(gists);
    AddConstStates(generatedStates);

    // We're done preprocessing this query, so let's start a new round.
    FOREACH_TWL( iter, whichOnes ) {
        QueryID query = iter.query;

        queriesPreprocessing.Difference(query);
        queriesToPrepare.Union(query);
    } END_FOREACH;

    return true; // Generate token requests
}

bool GISTWayPointImp :: PrepareRoundPossible( CPUWorkToken& token ) {
    PDEBUG("GISTWayPointImp :: PrepareRoundPossible()");

    if( queriesToPrepare.IsEmpty() )
        return false;

    QueryIDSet curQueries = queriesToPrepare;
    queriesToPrepare.Difference(curQueries);
    queriesPreparing.Union(curQueries);

    // Prepare the work description.
    HistoryList lineage;
    GISTHistory hist(GetID(), -1);
    lineage.Insert(hist);

    QueryToGLAStateMap gistsToPrepare;
    FOREACH_EM( query, gist, gistStates ) {
        if( query.Overlaps( curQueries ) ) {
            QueryID key = query;
            GLAState value;
            value.copy( gist );

            gistsToPrepare.Insert( key, value );
        }
    } END_FOREACH;

    QueryExitContainer qExits;

    while( !curQueries.IsEmpty() ) {
        QueryID temp = curQueries.GetFirst();
        QueryExit qe = GetExit(temp);
        qExits.Append(qe);
    }

    QueryExitContainer whichOnes;
    whichOnes.copy(qExits);

    GISTNewRoundWD workDesc(qExits, gistsToPrepare);

    WayPointID myID = GetID();
    WorkFunc myFunc = GetWorkFunction( GISTNewRoundWorkFunc :: type );

    myCPUWorkers.DoSomeWork( myID, lineage, whichOnes, token, workDesc, myFunc );

    return true;
}

bool GISTWayPointImp :: PrepareRoundComplete( QueryExitContainer& whichOnes,
        HistoryList& history, ExecEngineData& data ) {
    PDEBUG("GISTWayPointImp :: PrepareRoundComplete()");

    GISTNewRoundRez result;
    result.swap(data);

    QueryToGistWUContainer& newWorkUnits = result.get_workUnits();
    workUnits.SuckUp(newWorkUnits);

    // Mark the queries as done preparing and now processing.
    FOREACH_TWL(iter, whichOnes) {
        QueryID query = iter.query;

        queriesPreparing.Difference(query);

        FATALIF(!workUnits.IsThere(query),
                "Did not get back a list of work units for query %s",
                query.GetStr().c_str());
        GistWUContainer& workList = workUnits.Find(query);

        if( workList.Length() == 0 ) {
            // No work to be done
            queriesToCount.Union(query);
        }
        else {
            queriesProcessing.Union(query);
        }
    } END_FOREACH;

    return true; // more tokens
}

bool GISTWayPointImp :: ProcessingPossible( CPUWorkToken& token ) {
    PDEBUG("GISTWayPointImp :: ProcessingPossible ()");

    if( queriesProcessing.IsEmpty() )
        return false;

    QueryToGistWorkUnit curWorkUnits;
    QueryExitContainer qExits;

    // For now, just find a single work unit left to complete.
    bool foundWorkUnit = false;
    FOREACH_EM(query, workUnitList, workUnits) {
        if( workUnitList.IsEmpty() )
            continue;

        workUnitList.MoveToStart();
        GISTWorkUnit curWorkUnit;
        workUnitList.Remove( curWorkUnit );

        QueryID qID = query;
        curWorkUnits.Insert( qID, curWorkUnit );

        QueryExit qe = GetExit( query );
        qExits.Insert(qe);

        IncrementInt( processingJobsOut, query );

        foundWorkUnit = true;
        break;
    } END_FOREACH;

    // Make sure we found a work unit.
    if( !foundWorkUnit )
        return false;

    // Create the work description.
    HistoryList lineage;
    GISTHistory hist(GetID(), -1);
    lineage.Insert(hist);

    QueryExitContainer whichOnes;
    whichOnes.copy(qExits);

    GISTDoStepsWD workDesc( qExits, curWorkUnits );

    WayPointID myID = GetID();
    WorkFunc myFunc = GetWorkFunction( GISTDoStepsWorkFunc :: type );

    myCPUWorkers.DoSomeWork( myID, lineage, whichOnes, token, workDesc, myFunc );

    return true;
}

bool GISTWayPointImp :: ProcessingComplete( QueryExitContainer& whichOnes,
        HistoryList& history, ExecEngineData& data ) {
    PDEBUG("GISTWayPointImp :: ProcessingComplete ()");

    GISTDoStepRez result;
    result.swap(data);

    QueryToGistWorkUnit& unfinishedWork = result.get_unfinishedWork();
    QueryToGLAStateMap& finishedWork = result.get_finishedWork();

    FOREACH_TWL(iter, whichOnes) {
        QueryID query = iter.query;

        FATALIF( !workUnits.IsThere( query ),
                "Got back unfinished work for unknown query %s",
                query.GetStr().c_str());
        GistWUContainer& curWorkList = workUnits.Find( query );

        if( unfinishedWork.IsThere( query ) ) {
            GISTWorkUnit& work = unfinishedWork.Find( query );

            curWorkList.Append(work);
        }

        int numToMerge = 0;
        if( finishedWork.IsThere( query) ) {
            GLAState& gla = finishedWork.Find(query);

            if( !glasToMerge.IsThere(query) ) {
                GLAStateContainer newCont;
                QueryID key = query;
                glasToMerge.Insert(key, newCont);
            }

            GLAStateContainer& toMerge = glasToMerge.Find(query);
            toMerge.Append(gla);

            numToMerge = toMerge.Length();
        }

        if( DecrementInt( processingJobsOut, query ) == 0 && curWorkList.Length() == 0 ) {
            // We have no processing jobs out and no more work left.
            // Time to merge the glas!
            queriesProcessing.Difference(query);

            if( numToMerge > 1 ) {
                // If more than one GLA, merge them
                queriesMerging.Union(query);
            }
            else {
                // Otherwise, skip to counting
                FATALIF( numToMerge == 0, "Finished processing a round but have no GLAs!" );
                GLAStateContainer& toMerge = glasToMerge.Find(query);
                toMerge.MoveToStart();
                GLAState gla;
                toMerge.Remove(gla);

                QueryID key = iter.query;
                queriesToCount.Union(key);
                mergedGLAs.Insert(key, gla);
            }
        }
    } END_FOREACH;

    return true; // more tokens
}

bool GISTWayPointImp :: PostProcessingPossible( CPUWorkToken& token ) {
    PDEBUG("GISTWayPointImp :: PostProcessingPossible ()");

    if( queriesMerging.IsEmpty() )
        return false;

    QueryToGLASContMap toMerge;
    QueryExitContainer qExits;

    // Group together sets of GLAs, if possible.
    bool haveStatesToMerge = false;
    FOREACH_EM(query, mergeList, glasToMerge) {
        if( mergeList.Length() > 1 ) {
            GLAStateContainer curToMerge;
            mergeList.MoveToStart();

            for( int i = 0; i < MERGE_AT_A_TIME && mergeList.Length() != 0; ++i ) {
                GLAState temp;
                mergeList.Remove(temp);
                curToMerge.Append(temp);
            }

            QueryID key = query;
            toMerge.Insert( key, curToMerge );

            QueryExit qe = GetExit(query);
            qExits.Append(qe);

            IncrementInt( mergeJobsOut, query );

            haveStatesToMerge = true;
        }
    } END_FOREACH;

    if( !haveStatesToMerge )
        return false;

    // Create the work description
    HistoryList lineage;
    GISTHistory hist(GetID(), -1);
    lineage.Insert(hist);

    QueryExitContainer whichOnes;
    whichOnes.copy(qExits);

    GISTMergeStatesWD workDesc( qExits, toMerge );

    WayPointID myID = GetID();
    WorkFunc myFunc = GetWorkFunction( GISTMergeStatesWorkFunc :: type );

    myCPUWorkers.DoSomeWork( myID, lineage, whichOnes, token, workDesc, myFunc );

    return true;
}

bool GISTWayPointImp :: PostProcessingComplete( QueryExitContainer& whichOnes,
        HistoryList& history, ExecEngineData& data ) {
    PDEBUG("GISTWayPointImp :: PostProcessingComplete ()");

    GLAStatesRez result;
    result.swap(data);

    QueryToGLAStateMap& glaStates = result.get_glaStates();

    FOREACH_TWL(iter, whichOnes) {
        QueryID query = iter.query;

        FATALIF( !glaStates.IsThere(query),
                "Did not get back a GLA after a merge for query %s",
                query.GetStr().c_str());

        GLAState& curState = glaStates.Find(query);

        FATALIF( !glasToMerge.IsThere(query),
                "No list of GLAs to merge found for query %s",
                query.GetStr().c_str());

        GLAStateContainer& curMerge = glasToMerge.Find(query);

        if( DecrementInt(mergeJobsOut, query) == 0 && curMerge.IsEmpty() ) {
            // No more states to merge. This is the last one!
            // Time to pre-finalize
            queriesMerging.Difference(query);
            queriesToCount.Union(query);

            mergedGLAs.Insert( query, curState );
        }
        else {
            curMerge.Append(curState);
        }
    } END_FOREACH;

    return true; // more tokens
}

bool GISTWayPointImp :: PreFinalizePossible( CPUWorkToken& token ) {
    PDEBUG("GISTWayPointImp :: PreFinalizePossible ()");

    if( queriesToCount.IsEmpty() )
        return false;

    QueryIDSet curQueries = queriesToCount;
    queriesToCount.Difference(curQueries);
    queriesCounting.Union(curQueries);

    // Create the work description
    QueryToGLAStateMap curGLAs;
    QueryToGLAStateMap curGists;
    QueryExitContainer qExits;

    while( !curQueries.IsEmpty() ) {
        QueryID curID = curQueries.GetFirst();

        if( mergedGLAs.IsThere(curID) ) {
            QueryID key;
            GLAState state;
            mergedGLAs.Remove(curID, key, state);
            curGLAs.Insert( key, state );
        }

        FATALIF(!gistStates.IsThere(curID),
                "No GIST state found for query %s",
                curID.GetStr().c_str());

        GLAState& temp = gistStates.Find(curID);
        GLAState curGist;
        curGist.copy(temp);

        QueryID key = curID;
        curGists.Insert(key, curGist);

        QueryExit qe = GetExit(curID);
        qExits.Append(qe);
    }

    HistoryList lineage;
    GISTHistory hist(GetID(), -1);
    lineage.Insert(hist);

    QueryExitContainer whichOnes;
    whichOnes.copy(qExits);

    GISTShouldIterateWD workDesc(qExits, curGLAs, curGists);

    WayPointID myID = GetID();
    WorkFunc myFunc = GetWorkFunction( GISTShouldIterateWorkFunc :: type );

    myCPUWorkers.DoSomeWork( myID, lineage, whichOnes, token, workDesc, myFunc );

    return true;
}

bool GISTWayPointImp :: PreFinalizeComplete( QueryExitContainer& whichOnes,
        HistoryList& lineage, ExecEngineData& data ) {
    PDEBUG( "GISTWayPointImp :: PreFinalizeComplete ()");

    GISTShouldIterateRez result;
    result.swap(data);

    QueryIDToInt& fragInfo = result.get_fragInfo();
    QueryIDSet& qIter = result.get_queriesToIterate();

    queriesToIterate.Union(qIter);

    FOREACH_TWL(iter, whichOnes) {
        QueryID query = iter.query;
        queriesCounting.Difference(query);

        FATALIF(!fragInfo.IsThere(query),
                "No information about the amount of output found for query %s",
                query.GetStr().c_str());
        int numFrags = fragInfo.Find(query).GetData();

        if( numFrags > 0 ) {
            queryFragmentMap.ORAll( query, numFrags );
            queriesFinalizing.Union(query);

            Swapify<int> temp(numFrags);
            fragmentsLeft.Insert(query, temp);
        }
        else {
            FinishQuery( query );
        }

    } END_FOREACH;

    return true;
}

bool GISTWayPointImp :: FinalizePossible( CPUWorkToken& token ) {
    PDEBUG("GISTWayPointImp :: FinalizePossible ()");

    if( queriesFinalizing.IsEmpty() )
        return false;

    int fragmentNo;
    lastFragmentID = queryFragmentMap.FindFirstSet(lastFragmentID);
    if( lastFragmentID == -1 ) {
        lastFragmentID = 0; // no fragments found, start from beginning.
        return false;
    }
    else {
        fragmentNo = lastFragmentID;
    }

    Bitstring qryOut = queryFragmentMap.GetBits(fragmentNo);
    qryOut = qryOut.GetFirst();
    queryFragmentMap.Clear(fragmentNo, qryOut);

    QueryExit whichOne = GetExit(qryOut);

    QueryExitContainer whichOnes;
    QueryExit whichOneCopy = whichOne;
    whichOnes.Append(whichOneCopy);

    FATALIF(!gistStates.IsThere(qryOut),
            "Could not find GIST state for query %s",
            qryOut.GetStr().c_str());
    GLAState& myGist = gistStates.Find(qryOut);
    GLAState gistCopy;
    gistCopy.copy(myGist);

    // Create the work description.
    GISTProduceResultsWD workDesc(fragmentNo, whichOne, gistCopy);

    GISTHistory hist(GetID(), fragmentNo);
    HistoryList lineage;
    lineage.Insert(hist);

    WayPointID myID = GetID();
    WorkFunc myFunc;

    bool rezState = resultIsState.Find(qryOut).GetData();

    if( rezState )
        myFunc = GetWorkFunction( GISTProduceStateWorkFunc :: type );
    else
        myFunc = GetWorkFunction( GISTProduceResultsWorkFunc :: type );

    myCPUWorkers.DoSomeWork( myID, lineage, whichOnes, token, workDesc, myFunc );

    return true;
}

bool GISTWayPointImp :: FinalizeComplete( QueryExitContainer& whichOnes,
        HistoryList& lineage, ExecEngineData& data ) {
    PDEBUG("GISTWayPointImp :: FinalizeComplete ()");

    return true;
}

bool GISTWayPointImp :: ReceivedStartProducingMsg( HoppingUpstreamMsg& message, QueryExit& whichOne ) {
    PDEBUG("GISTWayPointImp :: ReceivedStartProducingMsg()");
    // Determine if we need to receive states for any queries.
    // If we do, send start producing messages to the terminating query exits
    // for that query.
    // Otherwise, send the query to preprocessing.
    QueryID query = whichOne.query;

    if( query.Overlaps(queriesCompleted) ) {
        RestartQuery(query);
        return true;
    }
    else {
        if( NumStatesNeeded(query) == 0)
            queriesToPreprocess.Union(query);
        else
            StartTerminatingExits(query);

        if( !queriesToPreprocess.IsEmpty() )
            return true;
        else
            return false;
    }

}

void GISTWayPointImp :: ProcessAckMsg( QueryExitContainer& whichOnes, HistoryList& lineage ) {
    PDEBUG("GISTWayPointImp :: ProcessAckMsg()");

    EXTRACT_HISTORY_ONLY(lineage, hist, GISTHistory);

    int fragNo = hist.get_whichFragment();
    QueryIDSet queries;

    FOREACH_TWL(iter, whichOnes) {
        QueryID query = iter.query;
        queries.Union(query);
        if( DecrementInt(fragmentsLeft, query) == 0 ) {
            // All fragments acknowledged.
            FinishQuery(query);
        }
    } END_FOREACH;

    LOG_ENTRY_P(2, "Fragment %s of %s query %s PROCESSED",
            fragNo, GetID().getName().c_str(), queries.GetStr().c_str());
}

void GISTWayPointImp :: ProcessDropMsg( QueryExitContainer& whichOnes, HistoryList& lineage ) {
    PDEBUG("GISTWayPointImp :: ProcessDropMsg()");

    EXTRACT_HISTORY_ONLY(lineage, hist, GISTHistory);

    int fragNo = hist.get_whichFragment();
    QueryIDSet queries;

    FOREACH_TWL(iter, whichOnes) {
        queries.Union(iter.query);
    } END_FOREACH;

    queryFragmentMap.OROne(fragNo, queries);

    LOG_ENTRY_P(2, "Fragment %d of %s query %s DROPPED",
            fragNo, GetID().getName().c_str(), queries.GetStr().c_str());

    // Need more tokens to resend the dropped stuff.
    GenerateTokenRequests();
}
