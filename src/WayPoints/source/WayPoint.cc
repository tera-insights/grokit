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

#ifndef _WAYPOINT_C_
#define _WAYPOINT_C_

#include "WayPoint.h"

#include <boost/algorithm/string/case_conv.hpp>

using namespace std;

WayPointID WayPoint :: GetID () {
    return data->GetID ();
}

void WayPoint :: swap (WayPoint &withMe) {

    PDEBUG ("WayPoint :: swap()");
    // this is the standard three-line swap
    WayPointImp *temp = data;
    data = withMe.data;
    withMe.data = temp;
}

WayPoint :: WayPoint () {
    PDEBUG ("WayPoint :: WayPoint()");
    data = new WayPointImp;
}

WayPoint :: ~WayPoint () {
    PDEBUG ("WayPoint :: ~WayPoint()");
    delete data;
}

void WayPoint :: GetEndingQueryExits (QueryExitContainer &putResHere) {
    PDEBUG ("WayPoint :: GetEndingQueryExits()");
    data->GetEndingQueryExits (putResHere);
}

void WayPoint :: GetFlowThruQueryExits (QueryExitContainer &putResHere) {
    PDEBUG ("WayPoint :: GetFlowThruQueryExits ()");
    data->GetFlowThruQueryExits (putResHere);
}

void WayPoint :: Configure (WayPointConfigureData &configData) {
    PDEBUG ("WayPoint :: Configure ()");

    // do the configuration
    WayPointImp *temp = data->Configure (configData);

    // if this results in a new waypoint (we went from generic to specific) then
    // kill the old one and switch over to the new one
    if (temp != data) {
        delete data;
        data = temp;
    }
}

void WayPoint :: RequestGranted (GenericWorkToken &returnVal) {
    PDEBUG ("WayPoint :: RequestGranted ()");
    data->RequestGranted (returnVal);
}

void WayPoint :: ProcessHoppingDataMsg (HoppingDataMsg &msg) {
    PDEBUG ("WayPoint :: ProcessHoppingDataMsg ()");
    data->ProcessHoppingDataMsg (msg);
}

void WayPoint :: ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message) {
    PDEBUG ("WayPoint :: ProcessHoppingDownstreamMsg ()");
    data->ProcessHoppingDownstreamMsg (message);
}

void WayPoint :: ProcessHoppingUpstreamMsg (HoppingUpstreamMsg &message) {
    PDEBUG ("WayPoint :: ProcessHoppingUpstreamMsg ()");
    data->ProcessHoppingUpstreamMsg (message);
}

void WayPoint :: ProcessAckMsg (QueryExitContainer &whichOnes, HistoryList &lineage) {
    PDEBUG ("WayPoint :: ProcessAckMsg ()");
    data->ProcessAckMsg (whichOnes, lineage);
}

void WayPoint :: ProcessDropMsg (QueryExitContainer &whichOnes, HistoryList &lineage) {
    PDEBUG ("WayPoint :: ProcessDropMsg ()");
    data->ProcessDropMsg (whichOnes, lineage);
}

void WayPoint :: ProcessDirectMsg (DirectMsg &message) {
    PDEBUG ("WayPoint :: ProcessDirectMsg ()");
    data->ProcessDirectMsg (message);
}

void WayPoint :: DoneProducing (QueryExitContainer &whichOnes, HistoryList &withMe,
        int returnVal, ExecEngineData& retData) {
    PDEBUG ("WayPoint :: DoneProducing ()");
    data->DoneProducing (whichOnes, withMe, returnVal, retData);
}


void WayPoint :: Debugg(void){
    PDEBUG ("WayPoint :: Debugg ()");
    data->Debugg();
}

void WayPoint :: ProcessServiceRequest(ServiceData& request) {
    PDEBUG("WayPoint :: ProcessServiceRequest ()");
    data->ProcessServiceRequest(request);
}

void WayPoint :: ProcessServiceControl(ServiceData& control) {
    PDEBUG("WayPoint :: ProcessServiceControl ()");

    std::string operation = control.get_kind();
    boost::to_lower(operation);

    if( operation == "stop" ) {
        data->ServiceStop(control);
    } else {
        data->ProcessUnknownServiceControl(control);
    }
}


#endif // _WAYPOINT_C_
