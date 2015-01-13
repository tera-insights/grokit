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

#include "WriterWayPointImp.h"
#include "CPUWorkerPool.h"
#include "Logging.h"

using namespace std;

WriterWayPointImp :: WriterWayPointImp () {}
WriterWayPointImp :: ~WriterWayPointImp () {}

void WriterWayPointImp :: ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message) {

    // check to see if someone has told us we are done
    if (!strcmp (message.get_msg ().TypeName (), "QueryDoneMsg")) {

        cerr << "Writer has been notified that all data is now out of the hash table.\n";

    }
}

void WriterWayPointImp :: DoneProducing (QueryExitContainer &whichOnes, HistoryList &history,
        int result, ExecEngineData& data) {
    SendAckMsg (whichOnes, history);
}

void WriterWayPointImp :: ProcessHoppingDataMsg (HoppingDataMsg &data) {

    // extract the waypoint-specific stuff from the message
    ExtractionContainer temp;
    temp.swap (data.get_data ());

    // see if a disk work token is available... for testing purposes, randomly drop 30%
    DiskWorkToken myToken;
    int drop = 0;
    if (!temp.get_diskTokenQueue ().AtomicRemove (myToken)) {
        drop = 1;
    }

    // note that if, for whatever reason, the writer does not use one of its tokens to
    // actually do some work, it still must take one of the disk work tokens it has been
    // sent (if one is available) and give it back... if it does not do this, then the
    // token could be lost forever
    if (!drop && drand48 () > .7) {
        GiveBackToken (myToken);
        drop = 1;
    }

    if (drop) {

        // if we do not get one, then we will just return a drop message to the sender
        SendDropMsg (data.get_dest (), data.get_lineage ());
        return;
    }

    // create the work spec and get it done!
    WriterWorkDescription workDesc (GetID (), temp.get_whichSegment (), temp.get_result ());

    // and send off the work request
    WayPointID myID;
    myID = GetID ();
    WorkFunc myFunc;
    myFunc = GetWorkFunction (WriterWorkFunc::type);
    myDiskWorkers.DoSomeWork (myID, data.get_lineage (), data.get_dest (), myToken, workDesc, myFunc);
}

