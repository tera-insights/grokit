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

#ifndef GT_WAY_POINT_IMP
#define GT_WAY_POINT_IMP

#include "ID.h"
#include "History.h"
#include "Tokens.h"
#include "WayPointImp.h"
#include "GLAData.h"
#include "GLAHelpers.h"
#include "Constants.h"
#include "GPWayPointImp.h"

class GTWayPointImp : public GPWayPointImp {

    /*
     * Class Members
     */

    // QueryIDSets for the various stages a query can be in.
    QueryIDSet queriesToPreprocess;
    QueryIDSet queriesProcessing;

    // container for cached GTs
    QueryToGLASContMap queryToGTs;

    /*
     * Overridden GPWayPointImp functions
     */

    bool PreProcessingPossible( CPUWorkToken& token );
    bool PreProcessingComplete( QueryExitContainer& whichOnes,
            HistoryList& history, ExecEngineData& data);

    void GotAllStates( QueryID query );

    void GotChunkToProcess( CPUWorkToken& token, QueryExitContainer& whichOnes,
            ChunkContainer& chunk, HistoryList& history );
    bool ProcessChunkComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data);

    bool ReceivedStartProducingMsg( HoppingUpstreamMsg& message, QueryExit& whichOne );

public:

    // Constructor and destructor
    GTWayPointImp ();
    virtual ~GTWayPointImp ();

    /*
     * Overridden WayPointImp functions
     */
    void TypeSpecificConfigure( WayPointConfigureData& configData );
};

#endif // GT_WAY_POINT_IMP
