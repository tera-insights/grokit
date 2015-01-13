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

#ifndef GP_WAY_POINT_IMP
#define GP_WAY_POINT_IMP

#include "ID.h"
#include "History.h"
#include "Tokens.h"
#include "WayPointImp.h"
#include "GLAData.h"
#include "GLAHelpers.h"
#include "Constants.h"
#include "WPFExitCodes.h"

/** WARNING: The chunk processing function has to return 0 and the
        finalize function 3 otherwise acknowledgments are not sent
        properly in the system
*/
class GPWayPointImp : public WayPointImp {
    // Map of QueryID to QueryExit
    typedef EfficientMap<QueryID, QueryExit> QueryIDToExitMap;
    QueryIDToExitMap queryIdentityMap;

    // Constant states used by some Queries
    QueryToGLAStateMap constStates;

    // States received from other waypoints
    QueryToGLASContMap requiredStates;

    // A counter for each query representing how many state objects that GLA is
    // waiting on to begin processing.
    QueryIDToInt statesNeeded;

    // Used to keep track of which constant states go where in the constStates list.
    typedef std::map< WayPointID, int > ReqStateIndexMap;
    typedef std::map< QueryID, ReqStateIndexMap > QueryToReqStateIndexMap;
    QueryToReqStateIndexMap constStateIndex;

    // Initializes information about constant states from configuration data.
    void InitReqStates( QueryToReqStates& reqStates );

protected:

    QueryExit GetExit( QueryID qID );

    /*************************************************************************/
    // The following methods are used by subclasses to access information about
    // constant states that are needed.
    /*************************************************************************/

    void AddConstStates( QueryToGLAStateMap& states );
    void GetConstStates( QueryToGLAStateMap& putHere );
    QueryToGLAStateMap GetConstStates();

    // Returns the number of states needed for a particular query.
    int NumStatesNeeded( QueryID query );

    // Returns a copy of the constant states map.
    QueryToGLASContMap GetReqStates();
    void GetReqStates( QueryToGLASContMap& putHere );

    typedef WPFExitCode ExitCode;

    // Called when a wayoint has received a state from another waypoint.
    virtual void GotState( StateContainer& state );

    // Called when a waypoint has received all of the states it needs for a query.
    virtual void GotAllStates( QueryID query ) = 0;

    // Removes data pertaining to the specified queries from the waypoint.
    void RemoveQueryData( QueryIDSet queries );

    // Sends start producing messages to all terminating query exits for a set of queries.
    void StartTerminatingExits( QueryIDSet queries );

    // This method is called when the waypoint has received a data message
    // containing a chunk to be processed and a work token has been acquired.
    // This method should generate the necessary work description and other
    // necessary information to perform work on the chunk.
    virtual void GotChunkToProcess( CPUWorkToken& token, QueryExitContainer& whichOnes,
            ChunkContainer& chunk, HistoryList& lineage );
    virtual bool ProcessChunkComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data);

    /*************************************************************************/
    // Methods for the various stages of the waypoint.
    /*************************************************************************/

    virtual bool PreProcessingPossible( CPUWorkToken& token );
    virtual bool PreProcessingComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data);

    virtual bool PrepareRoundPossible( CPUWorkToken& token );
    virtual bool PrepareRoundComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data);

    virtual bool ProcessingPossible( CPUWorkToken& token );
    virtual bool ProcessingComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data);

    virtual bool PostProcessingPossible( CPUWorkToken& token );
    virtual bool PostProcessingComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data );

    virtual bool PreFinalizePossible( CPUWorkToken& token );
    virtual bool PreFinalizeComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data );

    virtual bool FinalizePossible( CPUWorkToken& token );
    virtual bool FinalizeComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data );

    virtual bool PostFinalizePossible( CPUWorkToken& token );
    virtual bool PostFinalizeComplete( QueryExitContainer& whichOnes, HistoryList& history, ExecEngineData& data );

    /*************************************************************************/
    // The following methods are for handling other types of message received
    // by the waypoint, such as Query Done and Start Producing messages.
    // These methods must be defined by the subtype.
    /*************************************************************************/

    // This method is called when a query done message is received from another
    // waypoint. whichOnes contains the query exits from the message.
    // This method should return true if additional cpu tokens should be
    // generated.
    virtual bool ReceivedQueryDoneMsg( QueryExitContainer& whichOnes );

    // This method is called when a start producing message is received from
    // another waypoint downstream. whichOne is the query that is being
    // requested to start producing. This method should return true if
    // additional tokens should be generated.
    virtual bool ReceivedStartProducingMsg( HoppingUpstreamMsg& message, QueryExit& whichOne );

    /*************************************************************************/
    // The following methods are used for configuring the GLAWayPoint
    /*************************************************************************/

    // This method sets which exit code in DoneProducing to forward on the data received.
    // The data from any other return code is discarded.
    void SetResultExitCode( ExitCode exitCode );

    // Handles generic configuration functions for these kinds of waypoints.
    // Should be called by the specific waypoint's TypeSpecificConfigure
    void Configure( WayPointConfigureData& configData );

private:

    ExitCode resultExitCode;

public:

    // constructor and destructor
    GPWayPointImp ();
    virtual ~GPWayPointImp ();


    // these are just implementations of the standard WayPointImp functions

    /* Meaning of the return values:
     * -1 - Preprocessing
     *  0 - Chunk Processed
     *  1 - Post Processing
     *  2 - Pre-Finalize
     *  3 - Finalize
     *  4 - Post-Finalize
     */
    void DoneProducing (QueryExitContainer &whichOnes, HistoryList &history, int returnVal, ExecEngineData& data);
    void RequestGranted (GenericWorkToken &returnVal);
    void ProcessHoppingDataMsg (HoppingDataMsg &data);
    void ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message);
    void ProcessHoppingUpstreamMsg( HoppingUpstreamMsg& message);

    virtual void TypeSpecificConfigure( WayPointConfigureData& config ) = 0;
};

#endif
