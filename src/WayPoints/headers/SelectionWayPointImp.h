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

#ifndef SELECTION_WAY_POINT_IMP
#define SELECTION_WAY_POINT_IMP

#include "WayPointImp.h"
#include "GPWayPointImp.h"

// this class is super-simple, since selection can use the default implementation of almost everything
class SelectionWayPointImp : public GPWayPointImp {

    /*
     * Class Members
     */

    // QueryIDSets for the various stages a query can be in.
    QueryIDSet queriesToPreprocess;
    QueryIDSet queriesProcessing;

    /*
     * Overridden GPWayPointImp functions
     */

    bool PreProcessingPossible( CPUWorkToken& token );
    bool PreProcessingComplete( QueryExitContainer& whichOnes,
            HistoryList& history, ExecEngineData& data);

    void GotAllStates( QueryID query );

    void GotChunkToProcess( CPUWorkToken& token, QueryExitContainer& whichOnes,
            ChunkContainer& chunk, HistoryList& history );
    // Don't need to override ProcessChunkComplete, we're just going to pass on the
    // result anyways.

    bool ReceivedStartProducingMsg( HoppingUpstreamMsg& message, QueryExit& whichOne );

public:

    // const and destr
    SelectionWayPointImp ();
    virtual ~SelectionWayPointImp ();

    void TypeSpecificConfigure( WayPointConfigureData& config );
};

#endif // SELECTION_WAY_POINT_IMP
