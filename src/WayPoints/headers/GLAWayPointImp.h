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
#ifndef GLA_WAY_POINT_IMP
#define GLA_WAY_POINT_IMP

#include "ID.h"
#include "History.h"
#include "Tokens.h"
#include "GPWayPointImp.h"
#include "GLAData.h"
#include "GLAHelpers.h"
#include "Constants.h"

/** WARNING: The chunk processing function has to return 0 and the
        finalize function 3 otherwise acknowledgments are not sent
        properly in the system
*/
class GLAWayPointImp : public GPWayPointImp {
    //a constant just to allow changing number of states to be merged at a time
    static const int MERGE_AT_A_TIME = 2;

    // container for states
    QueryToGLASContMap myQueryToGLAStates;

    // States that were merged, need to finalize them
    QueryToGLAStateMap mergedStates;

    // States that need to be garbage collected.
    QueryToGLAStateMap garbageStates;

    QueryFragmentMap queryFragmentMap;

    // the fragments we still have to produce for each query
    // when counter is at 0, we are done
    QueryIDToInt fragmentsLeft;

    // A counter for each query representing if the merge is in progress or not
    // 0 = done, >0 = in progress
    QueryIDToInt mergeInProgress;

    // last fragment we generated to ensure a circular list behavior
    off_t lastFragmentId;

    // Keeps track of which queries produce their GLA as a state.
    QueryIDToBool resultIsState;

    // Some QueryIDSets to keep track of which queries are in which state.
    QueryIDSet queriesToPreprocess;
    QueryIDSet queriesProcessing;
    QueryIDSet queriesMerging;
    QueryIDSet queriesCounting;
    QueryIDSet queriesFinalizing;
    QueryIDSet queriesToPostFinalize;
    QueryIDSet queriesCompleted;

    // QueryIDSets to keep track of information about the GLA in each query.
    QueryIDSet queriesToRestart;

    // Queries that should produce intermediate results when iterating
    QueryIDSet queriesProducingIntermediates;

    typedef EfficientMap<QueryID, HoppingUpstreamMsg> QueryIDToUpstreamMsg;
    QueryIDToUpstreamMsg cachedProducingMessages;

    // Helper methods
    //bool MergeDone();
    void FinishQueries( QueryIDSet queries );
    void RestartQueries( QueryIDSet queries );

    // Overwritten virtual methods
    void GotChunkToProcess( CPUWorkToken & token, QueryExitContainer& whichOnes, ChunkContainer& chunk, HistoryList& lineage);

    void GotAllStates( QueryID query );

    virtual bool PreProcessingPossible( CPUWorkToken& token );
    virtual bool PostProcessingPossible( CPUWorkToken& token );
    virtual bool PreFinalizePossible( CPUWorkToken& token );
    virtual bool FinalizePossible( CPUWorkToken& token );
    virtual bool PostFinalizePossible( CPUWorkToken & token );

    virtual bool PreProcessingComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data );
    virtual bool ProcessChunkComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data);
    virtual bool PostProcessingComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data );
    virtual bool PreFinalizeComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data );
    virtual bool FinalizeComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data );
    virtual bool PostFinalizeComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data );

    virtual bool ReceivedQueryDoneMsg( QueryExitContainer& whichOnes );
    virtual bool ReceivedStartProducingMsg( HoppingUpstreamMsg& message, QueryExit& whichOne );

public:

    // constructor and destructor
    GLAWayPointImp ();
    virtual ~GLAWayPointImp ();


    virtual void ProcessAckMsg (QueryExitContainer &whichOnes, HistoryList &lineage);
    virtual void ProcessDropMsg (QueryExitContainer &whichOnes, HistoryList &lineage);
    virtual void TypeSpecificConfigure (WayPointConfigureData &configData);

};

#endif
