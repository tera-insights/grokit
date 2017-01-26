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
// for Emacs -*- c++ -*-

#ifndef _WAYPOINT_IMP_H_
#define _WAYPOINT_IMP_H_

#include <ctime>
#include <map>
#include <utility>

#include "WayPoint.h"
#include "ServiceData.h"

// Let's define some useful macros for subclasses to use.

// Converts the object "from" to "type" in variable "to" using swap()
// Note: "to" does not need to be declared beforehand.
#define CONVERT_SWAP(from, to, type) \
    type to; \
    ( to ).swap((from)); \
    FATALIF(!(to).IsValid(), "Invalid swap conversion between types! Expected type: %s", #type);

// Checks to make sure that exactly one history item of type "type" is present
// in the history list "lineage" and then extracts that history item into
// "item" using swapping.
// Note: "item" does not need to be declared beforehand.
#define EXTRACT_HISTORY_ONLY(lineage, item, type) \
    (lineage).MoveToStart (); \
    FATALIF ((lineage).RightLength () != 1 || !CHECK_DATA_TYPE (lineage.Current (), type), \
            "Got a bad lineage item sent to a waypoint!"); \
    type item; \
    (item).swap((lineage).Current());

#define EXTRACT_HISTORY_LAST(lineage, item, type) \
    (lineage).MoveToFinish(); \
    (lineage).Retreat(); \
    FATALIF(! CHECK_DATA_TYPE((lineage).Current(), type), "Wrong history type at end of lineage."); \
    type item; \
    item.swap((lineage).Current());

#define PUTBACK_HISTORY(lineage, item) \
    item.swap((lineage).Current());

// Checks to see if a given lineage contains a TableHistory at the end, and if
// so, logs the fact that a chunk is being processed by "processor"
// Saves the chunk ID in a variable named by the third argument.
#define CHECK_FROM_TABLE_AND_LOG_WITH_ID(lineage, processor, chunkID) \
    (lineage).MoveToFinish(); \
    (lineage).Retreat(); \
    if (CHECK_DATA_TYPE((lineage).Current(), TableHistory)) { \
        TableHistory hLin; \
        hLin.swap((lineage).Current()); \
        ChunkID& id = hLin.get_whichChunk(); \
        chunkID = id; \
        TableScanInfo infoTS; \
        id.getInfo(infoTS); \
        \
        LOG_ENTRY_P(2, "Chunk %d of %s Processed by %s", \
                id.GetInt(), infoTS.getName().c_str(), #processor) \
        hLin.swap((lineage).Current()); \
    }

// If you don't care about getting the chunk ID
#define CHECK_FROM_TABLE_AND_LOG(lineage, processor) \
{ \
    ChunkID dummy; \
    CHECK_FROM_TABLE_AND_LOG_WITH_ID(lineage, processor, dummy); \
}


// this is a generic WayPoint...
class WayPointImp {

private:

    // flag that tells us if this is a generic waypoint
    int isGeneric;

    // the identifier for this waypoint
    WayPointID myID;

    // the set of query exits that end at this waypoint
    QueryExitContainer myExits;

    // the set of query exits that pass thru this waypoint
    QueryExitContainer thruMe;

    // the set of work functions that are present in the waypoint
    WorkFuncContainer myFuncs;

    // keeps track of which work token types should be requested, and the
    // maximums and priorities for each
    typedef std::map<off_t, std::pair<int, int> > TokenRequestMap;
    TokenRequestMap tokensToRequest;

    // keeps track of the number of outstanding token requests for each type.
    std::map<off_t, int> tokenRequestsOut;

public:

    /***************************************************************************/
    // this first set of functions is provided by the basic WayPoint/WayPointImp
    // class, and should not be re-implemented by any particular "Imp" class
    /***************************************************************************/

    // gets the ID
    WayPointID GetID ();

    // Gets the name of the waypoint
    std::string GetName();

    // constructor; creates a dummy wayPoint of no particular type
    WayPointImp ();

    // destructor
    virtual ~WayPointImp ();

    // returns all of the query-exits that end at this waypoint
    void GetEndingQueryExits (QueryExitContainer &putResHere);

    // gives back a token without using it
    void GiveBackToken (GenericWorkToken &returnVal);

    // returns all of the query-exits that flow through this waypoint on their way to some dest
    void GetFlowThruQueryExits (QueryExitContainer &putResHere);

    // requests a specific type of token for immediate use; if request is accepted, return val is 1; if
    // not, then the return val is zero.  The name of the desired token type is passed as a string (ex:
    // CPUWorkToken::type or DiskWorkToken::type)
    int RequestTokenImmediate (off_t requestType, GenericWorkToken &returnVal, int priority = 1); // by default we use the highest priority

    // like above, but the token can be returned at a later time via a callback to the waypoint
    // by default we use the highest priority
    void RequestTokenDelayOK (off_t requestType, timespec minStart, int priority = 1);
    
    // Helper function for RequestTokenDelayOK that sets minStart = now
    void RequestTokenNowDelayOK(off_t requestType, int priority = 1);

    // the following five methods provide five different ways to send messages to/beween waypoints.  There is
    // a sixth way to send a message (the so-called "hopping data message") but this is always sent
    // asynchronously to the exec engine via an event processor

    // Send a data message downstream to everyone listed as a destination
    void SendHoppingDataMsg( QueryExitContainer& whichOnes, HistoryList& lineage, ExecEngineData& data );

    // send a message downstream in the graph to everyone listed as a desitination
    void SendHoppingDownstreamMsg (HoppingDownstreamMsg &msg);

    // send a message upstream to the waypoint (or waypoints) producing data for a specific query exit
    void SendHoppingUpstreamMsg (HoppingUpstreamMsg &msg);

    // send an ack message to all waypoints listed in lineage (the message is delivered in order of the
    void SendAckMsg (QueryExitContainer &whichOnes, HistoryList &lineage);

    // like above, but send a drop message
    void SendDropMsg (QueryExitContainer &whichOnes, HistoryList &lineage);

    // send a direct message to a particular waypoint
    void SendDirectMsg (DirectMsg &message);

    // tell the execution engine not to accept any resource requests that do not have
    // a priority value that is a number that is greater than the given cutoff
    // "request type" is the type of token request that you are setting the cutoff for
    void SetPriorityCutoff (off_t requestType, int priority);

    // see what the current priority cutoff is for a particular resource type
    int GetPriorityCutoff (off_t requestType);

    // this method configures the wayPoint... it is called either to (a) transform
    // the waypoint into one of an appropriate type and load it up with the code
    // that is is supposed to run and/or new metadata, or to (b) reload it with new
    // new code and/or metadata
    WayPointImp *Configure (WayPointConfigureData &configData);

    // waypoints have function pointers of type WorkFunc that are passed to them via
    // WayPointConfigureData objects.  These functions are used to do the actual work
    // of the waypoint.  This method allows a lookup of a particular function.  The
    // string sent as an arg to this method should match the string name of some
    // WorkFuncWrapper data type (JoinLHSWorkFunc::type, AggWorkFunc::type, etc.; see
    // "WayPointConfigureData.h.m4" for all of the WorkFuncWrapper data types)
    WorkFunc GetWorkFunction (off_t whichOne);

    // this can be called from within DoneProducing to reclaim the token that was
    // used to produce the data
    void ReclaimToken (GenericWorkToken &putResHere);

    /***************************************************************************/
    // this second set of functions is provided by the basic WayPoint/WayPointImp
    // class, but can be re-defined for any particular derived class
    /***************************************************************************/

    // this processes a notifcation from the execution engine that one of the waypoint's
    // hopping data messages has made it back into the sytem (even if there was not any
    // actual data in the message).  returnVal is the return val from the function run by
    // the CPU worker that was used to produce the hopping data.  The default implementation
    // does nothing
    // (Alin) returned ExecEngineData is passed as well for
    // post-processing. Can be changed with swap()
    virtual void DoneProducing (QueryExitContainer &whichOnes, HistoryList &history,
        int returnVal, ExecEngineData& data);

    // this is sort of a "callback function" that will be called by the execution engine
    // in response to a call to RequestWorkerDelayOK above.  One RequestWorkerDelayOK is
    // called, the caller is guaranteed to at some time in the future have its request granted
    virtual void RequestGranted (GenericWorkToken &returnVal);

    // this method configures the waypoint by extracting any waypoint-type-specific
    // info from the WayPointConfigureData object... invoked by the Configure method...
    // this method is only over-rided if there is non-generic configuration to do
    virtual void TypeSpecificConfigure (WayPointConfigureData &configData);

    // these next six functions handle the six types of messages defined above.  A
    // particular type of waypoint can choose to over-ride one or more of these functions,
    // or to use the basic implementation.

    // most waypoints will process a hopping data message as follows.  First, they will
    // go to the execution engine and ask for a work token.  Assuming they want to
    // be notified with an ack or a drop message regarding this data object, they will
    // append their own lineage object to the HistoryList; using the token, they will
    // ask for a worker to process the data, and send the result back to the exec engine,
    // along with the new lineage object.  The default imp just ignores the message.
    virtual void ProcessHoppingDataMsg (HoppingDataMsg &data);

    // The first function processes a downstream message to a bunch of query exits, and the
    // second produces an upstream message to the waypoint producing data for a particular
    // query exit... the default implementation for both just forwards the message on.
    // Most implementations will look at the message, process it in some way, and then forward
    // the message on.
    virtual void ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message);
    virtual void ProcessHoppingUpstreamMsg (HoppingUpstreamMsg &message);

    // The default implementation simply forwards the ack or drop on.  Since a waypoint will
    // only receive an ack or a drop if it added its own lineage info to the message, most
    // implementations will remove the lineage information that the waypoint added in the
    // first place, examine it, and then decide what to do---re-produce dropped data, note
    // that a chunk reached its final destination, etc.
    virtual void ProcessAckMsg (QueryExitContainer &whichOnes, HistoryList &lineage);
    virtual void ProcessDropMsg (QueryExitContainer &whichOnes, HistoryList &lineage);

    // the basic implementation here ignores the message and does nothing.  Most specific
    // implementations will take some action upon receiving the message
    virtual void ProcessDirectMsg (DirectMsg &message);

    // method to dump whatever the waypoint wants in case of a debugg
    // redefine this method in waypoints that need debugging
    virtual void Debugg(void){ }

    // Methods for services

    // The waypoint received a request for a service to perform some work.
    // The default implementation throws an error.
    virtual void ProcessServiceRequest(ServiceData& request);

    // Controls
    virtual void ProcessUnknownServiceControl(ServiceData& control);
    virtual void ServiceStop(ServiceData& request);

protected:
    void SendServiceReply(ServiceData& reply);
    void SendServiceInfo(std::string& serviceID, std::string& status, Json::Value& data);

    // Helper method to send a query done message for a given set of query exits
    void SendQueryDoneMsg( QueryExitContainer &whichOnes );

    // Helper method to send a start producing message for a query exit
    void SendStartProducingMsg( QueryExit whichOne );

    // Helper method that sets the number of particular kind of work token that
    // is desired by the waypoint. This will determine how many tokens of each
    // type that the waypoint will attempt to acquire when GenerateTokenRequests
    // is called.
    void SetTokensRequested( off_t requestType, int numTokens, int priority = 1 );

    // Returns the number of tokens that will be requested for the given request type
    // when GenerateTokenRequests is called.
    int GetTokensRequested( off_t requestType ) const;

    // Lets the waypoint know that a request for a particular token type was
    // completed successfully, decrementing the number of outstanding requests.
    void TokenRequestCompleted( off_t requestType );

    // Helper method to automatically generate token requests up to a maximum
    // number for certain types as specified by SetTokensRequested.
    void GenerateTokenRequests( void );

    // Generates token requests for a specific type up to the maximum
    // specified by SetTokensRequested;
    void GenerateTokenRequests( off_t requestType );
};

#endif // _WAYPOINT_IMP_H_
