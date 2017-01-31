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

#include "WayPointImp.h"
#include "ExecEngine.h"
#include "JoinWayPointImp.h"

using namespace std;

WayPointID WayPointImp :: GetID () {
    return myID;
}

string WayPointImp :: GetName() {
    return myID.getName();
}

WayPointImp :: WayPointImp () :
    isGeneric(1),
    myID(),
    myExits(),
    thruMe(),
    myFuncs(),
    tokensToRequest(),
    tokenRequestsOut()
{
}

WayPointImp :: ~WayPointImp () {
}

void WayPointImp :: DoneProducing (QueryExitContainer &doneWithUs, HistoryList &lineage,
    int returnVal, ExecEngineData& data) {
    // default... do nothing!
}

void WayPointImp :: GiveBackToken (GenericWorkToken &returnVal) {
    executionEngine.GiveBackToken (returnVal);
}

void WayPointImp :: ReclaimToken (GenericWorkToken &getMe) {
    executionEngine.ReclaimToken (getMe);
}

void WayPointImp :: GetEndingQueryExits (QueryExitContainer &putResHere) {
    QueryExitContainer output;
    for (myExits.MoveToStart (); myExits.RightLength (); myExits.Advance ()) {
        QueryExit temp;
        temp.copy (myExits.Current ());
        output.Insert (temp);
    }
    output.swap (putResHere);
}

void WayPointImp :: GetFlowThruQueryExits (QueryExitContainer &putResHere) {
    QueryExitContainer output;
    for (thruMe.MoveToStart (); thruMe.RightLength (); thruMe.Advance ()) {
        QueryExit temp;
        temp.copy (thruMe.Current ());
        output.Insert (temp);
    }
    output.swap (putResHere);
}

int WayPointImp :: RequestTokenImmediate (off_t requestType, GenericWorkToken &returnVal, int priority) {
    WayPointID temp = myID;
    return executionEngine.RequestTokenImmediate (temp, requestType, returnVal, priority);
}

void WayPointImp :: RequestTokenDelayOK (off_t requestType, schedule_time minStart, int priority) {
    WayPointID temp = myID;
    executionEngine.RequestTokenDelayOK (temp, requestType, minStart, priority);
}

void WayPointImp :: RequestTokenNowDelayOK(off_t rt, int p) {
  RequestTokenDelayOK(rt, RateLimiter::GetNow(), p);
}

void WayPointImp :: SendHoppingDataMsg( QueryExitContainer& whichOnes, HistoryList& lineage, ExecEngineData& data ) {
    WayPointID idCopy;
    idCopy.copy(myID);
    HoppingDataMsg message( myID, whichOnes, lineage, data );
    executionEngine.SendHoppingDataMsg( message );
}

void WayPointImp :: SendHoppingDownstreamMsg (HoppingDownstreamMsg &msg) {
    executionEngine.SendHoppingDownstreamMsg (msg);
    PDEBUG("WayPointImp :: SendHoppingDownstreamMsg (HoppingDownstreamMsg &msg)");
}

void WayPointImp :: SendHoppingUpstreamMsg (HoppingUpstreamMsg &msg) {
    executionEngine.SendHoppingUpstreamMsg (msg);
    PDEBUG("WayPointImp :: SendHoppingUpstreamMsg (HoppingUpstreamMsg &msg)");
}

void WayPointImp :: SendAckMsg (QueryExitContainer &whichOnes, HistoryList &lineage) {
    executionEngine.SendAckMsg (whichOnes, lineage);
}

void WayPointImp :: SendDropMsg (QueryExitContainer &whichOnes, HistoryList &lineage) {
    executionEngine.SendDropMsg (whichOnes, lineage);
}

void WayPointImp :: SendDirectMsg (DirectMsg &message) {
    executionEngine.SendDirectMsg (message);
}

void WayPointImp :: SetPriorityCutoff (off_t requestType, int priority) {
    executionEngine.SetPriorityCutoff (requestType, priority);
}

int WayPointImp :: GetPriorityCutoff (off_t requestType) {
    executionEngine.GetPriorityCutoff (requestType);
}

void WayPointImp :: RequestGranted (GenericWorkToken &returnVal) {
    FATAL ("Sent a worker to a generic waypoint.  How can it even ask for work?");
}

WorkFunc WayPointImp :: GetWorkFunction (off_t whichOne) {

    // try to fund the requested function
    for (myFuncs.MoveToStart (); myFuncs.RightLength (); myFuncs.Advance ()) {
        if (whichOne == myFuncs.Current ().Type ()) {
            return myFuncs.Current ().get_myFunc ();
        }
    }

    FATAL ("BAD! Could not find the function you asked for.\n");
}

// forward definition of the waypoint factory
// keeps the number of files low
WayPointImp* WayPointFactory(const WayPointConfigureData &configData);

WayPointImp* WayPointImp :: Configure (WayPointConfigureData &configData) {

    WayPointImp *returnVal;

    // first see if this is not a generic waypoint... if not, then
    // we have been configured before, and this is just an update
    if (!isGeneric) {
        returnVal = this;

    // in this case we have NOT been configured, so create the appropriate type
    } else {
        // must create a new waypoint
        // NOTE: if you are adding a new type of waypoint, follow the
        // instructions in WayPointFactory.cc
        returnVal = WayPointFactory(configData);
    }

    // note that this is NOT a generic waypoint any more
    isGeneric = 0;

    // now that we have the way point imp to configure, do the generic stuff
    // first, load up the query routing information and the identifier
    configData.get_endingQueryExits ().swap (returnVal->myExits);
    configData.get_flowThroughQueryExits ().swap (returnVal->thruMe);
    configData.get_myID ().swap (returnVal->myID);

    // now load up the work functions
    WorkFuncContainer &allFuncs = configData.get_funcList ();
    for (allFuncs.MoveToStart (); allFuncs.RightLength (); allFuncs.Advance ()) {

        // see if this function is there
        int isThere = 0;
        for (returnVal->myFuncs.MoveToStart (); returnVal->myFuncs.RightLength (); returnVal->myFuncs.Advance ()) {
            if (allFuncs.Current ().Type() ==  returnVal->myFuncs.Current ().Type()) {

                // found him already in the waypoint!  So replace him
                returnVal->myFuncs.Current ().swap (allFuncs.Current ());
                isThere = 1;
                break;
            }
        }

        // if he is not already there, then add him
        if (!isThere) {
            WorkFuncWrapper temp;
            allFuncs.Current ().swap (temp);
            returnVal->myFuncs.Insert (temp);
        }
    }

    // now do the type specific configure
    returnVal->TypeSpecificConfigure (configData);

    // finally return this guy
    return returnVal;
}

void WayPointImp :: TypeSpecificConfigure (WayPointConfigureData &configData) {

    // default is to do nothing
}

void WayPointImp :: ProcessHoppingDataMsg (HoppingDataMsg &data) {

    // in generic case, die...
    FATAL ("How can you have a default processor for a hopping data message???");
}

void WayPointImp :: ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message) {

    // in generic case: just forward the message onwards
    SendHoppingDownstreamMsg (message);
}

void WayPointImp :: ProcessHoppingUpstreamMsg (HoppingUpstreamMsg &message) {

    // in generic case: just forward the message on
    SendHoppingUpstreamMsg (message);
}

void WayPointImp :: ProcessAckMsg (QueryExitContainer &whichOnes, HistoryList &lineage) {

    // in generic case, remove our history and move along
    History temp;
    lineage.MoveToFinish ();
    lineage.Retreat ();
    lineage.Remove (temp);
    SendAckMsg (whichOnes, lineage);
}

void WayPointImp :: ProcessDropMsg (QueryExitContainer &whichOnes, HistoryList &lineage) {

    // in generic case, remove our history and move along
    History temp;
    lineage.MoveToFinish ();
    lineage.Retreat ();
    lineage.Remove (temp);
    SendDropMsg (whichOnes, lineage);
}

void WayPointImp :: ProcessDirectMsg (DirectMsg &message) {
    // in generic case, do nothing
}

void WayPointImp :: SendQueryDoneMsg( QueryExitContainer &whichOnes ) {
    QueryExitContainer whichOnesCopy;
    whichOnesCopy.copy(whichOnes);
    QueryDoneMsg someAreDone (GetID(), whichOnes);
    HoppingDownstreamMsg myOutMsg (GetID (), whichOnesCopy, someAreDone);
    SendHoppingDownstreamMsg( myOutMsg );
}

void WayPointImp :: SendStartProducingMsg( QueryExit whichOne ) {
    QueryExit whichOneCopy = whichOne;

    StartProducingMsg startMsg( GetID(), whichOne );
    HoppingUpstreamMsg outMsg( GetID(), whichOneCopy, startMsg );
    SendHoppingUpstreamMsg( outMsg );
}

void WayPointImp :: SetTokensRequested( off_t requestType, int numTokens, int priority ) {
    tokensToRequest[requestType] = pair<int,int>( numTokens, priority );
}

int WayPointImp :: GetTokensRequested( off_t requestType ) const {
    return tokensToRequest.at(requestType).first;
}

void WayPointImp :: TokenRequestCompleted( off_t requestType ) {
    tokenRequestsOut[requestType] -= 1;
}

void WayPointImp :: GenerateTokenRequests( void ) {
    for( TokenRequestMap::iterator it = tokensToRequest.begin(); it != tokensToRequest.end(); ++it ) {
        off_t type = it->first;
        const int& maxReq = it->second.first;
        const int& priority = it->second.second;

        while( tokenRequestsOut[type] < maxReq ) {
            tokenRequestsOut[type] += 1;
            RequestTokenNowDelayOK( type, priority );
        }
    }
}

void WayPointImp :: GenerateTokenRequests( off_t requestType ) {
    TokenRequestMap::iterator it = tokensToRequest.find( requestType );

    if( it != tokensToRequest.end() ) {
        off_t type = it->first;
        const int& maxReq = it->second.first;
        const int& priority = it->second.second;

        while( tokenRequestsOut[type] < maxReq ) {
            tokenRequestsOut[type] += 1;
            RequestTokenNowDelayOK( type, priority );
        }
    }
}

void WayPointImp :: SendServiceReply(ServiceData& reply) {
    executionEngine.SendServiceReply(reply);
}

void WayPointImp :: SendServiceInfo(std::string& serviceID, std::string& status, Json::Value& data) {
    executionEngine.SendServiceInfo(serviceID, status, data);
}

void WayPointImp :: ProcessServiceRequest(ServiceData& request) {
    ServiceData reply = ServiceErrors::MakeError(
            request,
            ServiceErrors::OperationNotSupported,
            "Not able to process service requests in this waypoint",
            GetID().getName()
            );
    SendServiceReply(reply);
}

void WayPointImp :: ServiceStop(ServiceData& request) {
    ServiceData reply = ServiceErrors::MakeError(
            request,
            ServiceErrors::OperationNotSupported,
            "Stopping this service unsupported"
            );
    SendServiceReply(reply);
}

void WayPointImp :: ProcessUnknownServiceControl(ServiceData& control) {
    ServiceData reply = ServiceErrors::MakeError(
            control,
            ServiceErrors::NoSuchOperation,
            "No such operation known",
            control.get_kind()
            );
    SendServiceReply(reply);
}
