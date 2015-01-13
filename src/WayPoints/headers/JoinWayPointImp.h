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


#ifndef JOIN_IMP_H
#define JOIN_IMP_H

#include "WayPointImp.h"
#include "HashTable.h"

class JoinWayPointImp : public WayPointImp {
    private:
        enum State {
            DYING = 0,
            FINE,
            DEAD
        };

    public:

        typedef Swapify <int> SwapifiedInt;

    private:

        // used to remember whether or not we are "dying" (aka "wounded")... we are
        // dying/wounded if it is determined that this waypoint is taking up too much
        // space in the hash table and so it is to be extracted
        int state;

        // this is the unique join way point ID for this guy
        unsigned int myJoinWayPointID;

        // these are the RHS query exits we have finished up, but where we have not
        // yet started running the LHS
        QueryExitContainer doneRHS;

        // still going is the complete set of query exits that are either going into
        // or flowing through this wapyoint
        QueryExitContainer stillGoing;

        QueryExitContainer needToStart;

        // these are the LHS query exits we have held back until the RHS finishes
        TwoWayList <StartProducingMsg> waitingOnRHS;

        // this is the system hash table
        HashTable centralHashTable;
        bool hashTableReady;

        // this is the identifier of the cleaner waypoint
        WayPointID hashTableCleaner;

    public:

        // constructor and destructor
        JoinWayPointImp();
        ~JoinWayPointImp();

        // here we over-ride the standard WayPointImp functions
        void TypeSpecificConfigure (WayPointConfigureData &configData);
        void ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message);
        void DoneProducing (QueryExitContainer &whichOnes, HistoryList &history, int result, ExecEngineData &message);
        void ProcessHoppingDataMsg (HoppingDataMsg &message);
        void ProcessHoppingUpstreamMsg (HoppingUpstreamMsg &message);
        void ProcessDirectMsg (DirectMsg &message);
};


#endif
