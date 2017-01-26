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

#ifndef EXEC_ENGINE_IMP_H
#define EXEC_ENGINE_IMP_H

#include <string>
#include <unordered_map>
#include <queue>
#include <ctime>

#include "Tokens.h"
#include "EventProcessor.h"
#include "Message.h"
#include "EEMessageTypes.h"
#include "LineageData.h"
#include "WayPoint.h"
#include "ServiceData.h"
#include "ServiceMessages.h"

struct TokenRequest;

class ExecEngineImp : public  EventProcessorImp {

private:
    // this is the central message queue used to order all of the reuqests
    typedef Swapify <int> SwapifiedInt;
    TwoWayList <SwapifiedInt> requests;

    // the set of all CPU tokens that are not assigned
    TwoWayList <CPUWorkToken> unusedCPUTokens;

    // the set of all disk work tokens that are not assigned
    TwoWayList <DiskWorkToken> unusedDiskTokens;

    // these are all of the message queues
    TwoWayList <HoppingDataMsg> hoppingDataMessages;
    TwoWayList <HoppingDownstreamMsg> hoppingDownstreamMessages;
    TwoWayList <HoppingUpstreamMsg> hoppingUpstreamMessages;
    TwoWayList <LineageData> acks;
    TwoWayList <LineageData> drops;
    TwoWayList <DirectMsg> directMessages;

    // the routing graph
    DataPathGraph myGraph;

    // the set of waypoints
    WayPointMap myWayPoints;

    // this is the set of outstanding requests for disk and CPU tokens
    std::priority_queue <TokenRequest> requestListCPU;
    std::priority_queue <TokenRequest> requestListDisk;

    // this is the set of outstanding requests that are too low in priority to be fulfilled
    TwoWayList <TokenRequest> frozenOutFromCPU;
    TwoWayList <TokenRequest> frozenOutFromDisk;

    // this is the cutoff in priority for the CPU and the disk... a higher cutoff means that
    // it is harder to get te resources.  Any resource request having a priority that is a
    // larger number than the cutoff for the resource can never be fulfilled until the
    // priority cutoff changes to a value that is no smaller than the priority of the request
    int priorityCPU;
    int priorityDisk;

    // ask the execution engine to deliver some message
    int DeliverSomeMessage ();

    // these manipulate the central FIFO queue
    void InsertRequest (int requestID);
    int AreRequests ();
    void RemoveRequest (int &requestID);

    // this is the most recent token we got back from a worker... it is stored by the engine
    // momentarily so that a waypoint can ask for it back within the DoneProducing method
    GenericWorkToken holdMe;
    int holdMeIsValid;

    // Service registration and handling
    const std::string mailbox;
    EventProcessor serviceFrontend;

    using ServiceMap = std::unordered_map<std::string, WayPointID>;
    ServiceMap services;

protected:

    friend class ExecEngine;

    // Actor PreStart
    virtual void PreStart(void) override;

    // these functions are called by the ExecEngine class to actually send messages... they should NOT
    // be called by anyone else.  If a particular waypoint type wants to send a message, it should make
    // a call to the ExecEngine class and not call these functions directly
    void SendHoppingDataMsg( HoppingDataMsg &);
    void SendHoppingDownstreamMsg (HoppingDownstreamMsg &);
    void SendHoppingUpstreamMsg (HoppingUpstreamMsg &);
    void SendAckMsg (QueryExitContainer &, HistoryList &);
    void SendDropMsg (QueryExitContainer &, HistoryList &);
    void SendDirectMsg (DirectMsg &);

    // this one is similar to the above, except that it gives back a work token... it should also not be
    // called directly
    void GiveBackToken (GenericWorkToken &);

    // this one can be called by a waypoint when it is in the middle of DoneProducing to get back
    // the token that was given to the ExecEngine along with the hopping data message
    void ReclaimToken (GenericWorkToken &);

    // these next two should also only be called by the ExecEngine class...

    // request a work token from the execution engine; if one is available, a 1 is returned... otherwise,
    // a zero is returned... called when you cannot wait and will drop work if no worker is availbale
    // Note the priority field.  The smaller the number, the greater the priority associated with the
    // request being made.  The way that this works is that your request will be denied (even if
    // there are tokens available) if the priority cutoff for your request type has been set to be
    // a number that is less than your request's priority
    int RequestTokenImmediate (WayPointID &whoIsAsking, off_t requestType, GenericWorkToken &returnVal, int priority = 1);

    // request a work token for some future time... note that your request can never be granted until
    // the priority cutoff for your request type has been set to a number that is equal to or greater
    // than your request's priority
    void RequestTokenDelayOK (WayPointID &whoIsAsking, off_t requestType, timespec minStartTime, int priority = 1);

    // this sets the priority cutoff for a particular requet type (note a lower number means a higher
    // cutoff, since 1 is the highest priority).  The way that this works is that no token requests will
    // ever be granted for the particular request type if the priority associated with the request is
    // a greater number than the current cutoff
    void SetPriorityCutoff (off_t requestType, int priority);

    // this obtains the current priority cutoff
    int GetPriorityCutoff (off_t requestType);

    /*
     * Registers the specified waypoint as the handler for the service given by serviceID.
     * Any requests sent for that service will be forwarded to that waypoint.
     */
    bool RegisterService( WayPointID& wp, std::string& serviceID );

    /*
     * Removes a service from the system. Any further requests for this service will be
     * immediately rejected with an error.
     */
    bool RemoveService( std::string& serviceID );

    void SendServiceReply( ServiceData& reply );

    void SendServiceInfo( std::string& serviceID, std::string& status, Json::Value& data );

public:

    // constructor and destructor
    ExecEngineImp (const std::string & _mailbox);
    virtual ~ExecEngineImp ();

    // this function helps in debugging
    // it can be called form a message function or from GDB
    // it is unsafe to call it from any other code (race conditions)
    void Debugg(void);

    // this allows one to configure the execution engine
    MESSAGE_HANDLER_DECLARATION(ConfigureExecEngine);

    // this allows one to inject a hopping data message into the execution egine
    MESSAGE_HANDLER_DECLARATION(HoppingDataMsgReady);

    // allows someone to give back a token without a hopping data message
    MESSAGE_HANDLER_DECLARATION(GiveTokenBack);

    // allows someone to send a request to a registered service.
    MESSAGE_HANDLER_DECLARATION(ServiceRequestMessage_H);

    // allows someone to send a control message to a registered service.
    MESSAGE_HANDLER_DECLARATION(ServiceControlMessage_H);

    // Checks if there are any requests that we can now satisfy. Called ~10 times/second
    MESSAGE_HANDLER_DECLARATION(TickHandler);
};

// this is a silly little struct that is used to hold requests for resource tokens
struct TokenRequest {

    WayPointID whoIsAsking;
    int priority;
    timespec minStartTime;

    TokenRequest () {}
    ~TokenRequest () {}

    TokenRequest (WayPointID whoIn, int priorityIn, timespec minStartTimeIn) {
        whoIsAsking = whoIn;
        priority = priorityIn;
	minStartTime = minStartTimeIn;
    }

    void swap (TokenRequest &withMe) {
        char temp[sizeof (TokenRequest)];
        memmove (temp, &withMe, sizeof (TokenRequest));
        memmove (&withMe, this, sizeof (TokenRequest));
        memmove (this, temp, sizeof (TokenRequest));
    }
};

bool operator<(const timespec a, const timespec b) {
  if (a.tv_sec == b.tv_sec) {
    return a.tv_nsec < b.tv_nsec;
  }

  return a.tv_sec < b.tv_sec;
}

// Because we use a std::priority_queue<TokenRequest>, we want the "greatest"
// TokenRequest (i.e. the one popped first) to be the one with the soonest
// minStartDate.
// If a.tv_nsec > b.tv_nsec, then a < b. This makes sense:
// a.minStartTime is after b.minStartTime and we want to schedule
// b before a.
bool operator<(const TokenRequest a, const TokenRequest b) {
  return a.minStartTime > b.minStartTime;
}

#endif
