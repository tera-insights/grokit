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

#include "CompactWayPointImp.h"
#include "CPUWorkerPool.h"
#include "Logging.h"

using namespace std;

CompactWayPointImp :: CompactWayPointImp () :
    GPWayPointImp()
{
    PDEBUG("CompactWayPointImp :: CompactWayPointImp ()");
    SetResultExitCode( WP_PROCESS_CHUNK );
}
CompactWayPointImp :: ~CompactWayPointImp () {PDEBUG("CompactWayPointImp :: CompactWayPointImp ()");}

void CompactWayPointImp :: TypeSpecificConfigure( WayPointConfigureData& config ) {
    GPWayPointImp::Configure(config);
}

void CompactWayPointImp :: GotAllStates( QueryID query ) {
    PDEBUG( "CompactWayPointImp :: GotAllStates ()");

    GenerateTokenRequests();
}

void CompactWayPointImp :: GotChunkToProcess( CPUWorkToken& token,
        QueryExitContainer& whichOnes, ChunkContainer& chunk, HistoryList& history ) {
    PDEBUG( "CompactWayPointImp :: GotChunkToProcess()" );

    ChunkID chunkID;

    // if we have a chunk produced by a table waypoint log it
    CHECK_FROM_TABLE_AND_LOG_WITH_ID(history, Compact, chunkID);

    // create the work spec and get it done!
    QueryExitContainer myDestinations;
    myDestinations.copy (whichOnes);
    CompactProcessChunkWD workDesc (chunkID, myDestinations, chunk.get_myChunk());

    // and send off the work request
    WayPointID myID;
    myID = GetID ();
    WorkFunc myFunc;
    myFunc = GetWorkFunction (CompactProcessChunkWorkFunc::type);
    myCPUWorkers.DoSomeWork (myID, history, whichOnes, token, workDesc, myFunc);
}

