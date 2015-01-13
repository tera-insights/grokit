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
#ifndef GIST_WAY_POINT_IMP
#define GIST_WAY_POINT_IMP

#include "ID.h"
#include "History.h"
#include "Tokens.h"
#include "GPWayPointImp.h"
#include "GLAData.h"
#include "GLAHelpers.h"
#include "Constants.h"

class GISTWayPointImp : public GPWayPointImp {

    /***** Constants *****/

    // The maximum number of states to attempt to merge at a time.
    static const int MERGE_AT_A_TIME = 2;

    /***** Data Members *****/

    // Data from configuration
    QueryIDToBool resultIsState;

    /*
     * Query Stage Data
     * Keeps track of which queries are in which stage.
     */

    // Preprocessing stage
    QueryIDSet queriesToPreprocess;
    QueryIDSet queriesPreprocessing;
    QueryToGLAStateMap gistStates;

    // Round preparation stage
    QueryIDSet queriesToPrepare;
    QueryIDSet queriesPreparing;
    QueryToGistWUContainer workUnits;

    // Processing stage
    QueryIDSet queriesProcessing;
    QueryIDToInt processingJobsOut;

    // Merging stage
    QueryIDSet queriesMerging;
    QueryToGLASContMap glasToMerge;
    QueryIDToInt mergeJobsOut;

    // PreFinalize stage
    QueryIDSet queriesToCount;
    QueryIDSet queriesCounting;
    QueryToGLAStateMap mergedGLAs;

    // Finalize stage
    QueryIDSet queriesFinalizing;
    QueryFragmentMap queryFragmentMap;
    QueryIDToInt fragmentsLeft;
    QueryIDSet queriesToIterate;
    int lastFragmentID;

    // Completed
    QueryIDSet queriesCompleted;

    /***** Helper methods *****/

    // Used to determine when processing and merging are complete
    int DecrementInt( QueryIDToInt& count, QueryID query );
    int IncrementInt( QueryIDToInt& count, QueryID query );

    // Used when finishing and restarting queries
    void FinishQuery( QueryID query );
    void RestartQuery( QueryID query );

    /***** Overridden GPWayPointImp methods *****/

    void GotAllStates( QueryID query );

    bool PreProcessingPossible( CPUWorkToken& token );
    bool PreProcessingComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data);

    bool PrepareRoundPossible( CPUWorkToken& token );
    bool PrepareRoundComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data);

    bool ProcessingPossible( CPUWorkToken& token );
    bool ProcessingComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data);

    bool PostProcessingPossible( CPUWorkToken& token );
    bool PostProcessingComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data);

    bool PreFinalizePossible( CPUWorkToken& token );
    bool PreFinalizeComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data);

    bool FinalizePossible( CPUWorkToken& token );
    bool FinalizeComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data);

    bool ReceivedStartProducingMsg( HoppingUpstreamMsg& message, QueryExit& whichOne );

public:

    // Constructor and destructor
    GISTWayPointImp();
    virtual ~GISTWayPointImp();

    /***** Overridden WayPointImp methods *****/
    void ProcessAckMsg( QueryExitContainer& whichOnes, HistoryList& lineage );
    void ProcessDropMsg( QueryExitContainer& whichOnes, HistoryList& lineage );
    void TypeSpecificConfigure( WayPointConfigureData& configData );
};

#endif // GIST_WAY_POINT_IMP
