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

#ifndef _WAYPOINT_H_
#define _WAYPOINT_H_

#include "ID.h"
#include "WayPointConfigureData.h"
#include "EEMessageTypes.h"
#include "EEExternMessages.h"
#include "History.h"
#include "WayPointImp.h"
#include "Debug.h"
#include "ServiceData.h"

class WayPointImp;

// this is a generic WayPoint...
class WayPoint {

    protected:

        WayPointImp *data;

    public:

        /***************************************************************************/
        // this first set of functions is provided by the basic WayPoint/WayPointImp
        // class, and should not be re-implemented by any particular "Imp" class
        /***************************************************************************/

        // gets the ID
        WayPointID GetID ();

        // swaps two wayPoints
        void swap (WayPoint &withMe);

        // constructor; creates a dummy wayPoint of no particular type
        WayPoint ();

        // destructor
        ~WayPoint ();

        // returns all of the query-exits that end at this waypoint
        void GetEndingQueryExits (QueryExitContainer &putResHere);

        // returns all of the query-exits that flow through this waypoint on their way to some dest
        void GetFlowThruQueryExits (QueryExitContainer &putResHere);

        // this method configures the wayPoint... it is called either to (a) transform
        // the waypoint into one of an appropriate type and load it up with the code
        // that is is supposed to run and/or new metadata, or to (b) reload it with new
        // new code and/or metadata.  The default version of Configure handles all of the
        // data presrnt in the basic WayPointConfigureData object type... a specific waypoint
        // may implement its own TypeSpecificConfigure () method (see below) to handle any
        // additional data that is specific to that particular waypoint
        void Configure (WayPointConfigureData &configData);

        /***************************************************************************/
        // this second set of functions is provided by the basic WayPoint/WayPointImp
        // class, but can be re-defined for any particular derived class
        /***************************************************************************/

        // this method is called by Configure to handle any additional, type-specific
        // configuration... the default version of this function does nothing
        void TypeSepcificConfigure (WayPointConfigureData &configData);

        // this is sort of a "callback function" that will be called by the execution engine
        // in response to a call to RequestWorkerDelayOK above.  One RequestWorkerDelayOK is
        // called, the caller is guaranteed to at some time in the future have its request granted
        void RequestGranted (GenericWorkToken &returnVal);

        // this is called by the execution engine when a hopping data message that the waypoint
        // initiated has been produced and is moving through the system... all of the lineage
        // associated with the object is sent to the waypoint for processing
        void DoneProducing (QueryExitContainer &whichOnes, HistoryList &lineage,
                int returnVal, ExecEngineData& data);

        // these next six functions handle the six types of messages in the exec engine.  A
        // particular type of waypoint can choose to over-ride one or more of these functions,
        // or to use the basic implementation.

        // most waypoints will process a hopping data message as follows.  Assuming they want to
        // be notified with an ack or a drop message regarding this data object, they will
        // append their own lineage object to the HistoryList; they will then process the
        // data in some way, and then send the whole thing on with a call to SendHoppingDataMsg.
        // It is possible to use the default imlementation, which just forwards the message on
        void ProcessHoppingDataMsg (HoppingDataMsg &msg);

        // The first function processes a downstream message to a bunch of query exits, and the
        // second produces an upstream message to the waypoint producing data for a particular
        // query exit... the default implementation for both just forwards the message on.
        // Most implementations will look at the message, process it in some way, and then forward
        // the message on.
        void ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message);
        void ProcessHoppingUpstreamMsg (HoppingUpstreamMsg &message);

        // The default implementation simple forwards the ack or drop on.  Since a waypoint will
        // only receive an ack or a drop if it added its own lineage info to the message, most
        // implementations will remove the lineage information that the waypoint added in the
        // first place, examine it, and then decide what to do---re-produce dropped data, note
        // that a chunk reached its final destination, etc.
        void ProcessAckMsg (QueryExitContainer &whichOnes, HistoryList &lineage);
        void ProcessDropMsg (QueryExitContainer &whichOnes, HistoryList &lineage);

        // the basic implementation here ignores the message and does nothing.  Most specific
        // implementations will take some action upon receiving the message
        void ProcessDirectMsg (DirectMsg &message);

        // method to dump whatever the waypoint wants in case of a debugg
        void Debugg(void);


        // Methods for services
        void ProcessServiceRequest(ServiceData& request);
        void ProcessServiceControl(ServiceData& control);
};

typedef EfficientMap <WayPointID, WayPoint> WayPointMap;

#endif // _WAYPOINT_H_
