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

#include "JoinWayPointImp.h"
#include "CPUWorkerPool.h"
#include "HashTableCleanerWayPointImp.h"
#include "Properties.h"

using namespace std;

JoinWayPointImp :: JoinWayPointImp () :
    state(FINE),
    myJoinWayPointID(),
    doneRHS(),
    stillGoing(),
    needToStart(),
    waitingOnRHS(),
    centralHashTable(),
    hashTableReady(false),
    hashTableCleaner()
{
    PDEBUG ("JoinWayPointImp :: JoinWayPointImp ()");
}

JoinWayPointImp :: ~JoinWayPointImp () {
    PDEBUG ("JoinWayPointImp :: ~JoinWayPointImp ()");
}

void JoinWayPointImp :: ProcessDirectMsg (DirectMsg &message) {

    // check to see if someone has told us to die
    if (message.get_message ().Type() == IKillYouMessage::type) {

        cerr << "\n\n\n\nI have been killed!\n\n\n";
        if (state == FINE) {
            state = DYING;

            // if we are now dying, we just go ahead and start up all of the LHS queries
            cout << "starting the following: \n";
            for (waitingOnRHS.MoveToStart (); waitingOnRHS.RightLength (); ) {

                // remove the waiting notification
                StartProducingMsg startMsg;
                waitingOnRHS.Remove (startMsg);

                // and build the new hopping message that will start the LHS
                WayPointID curPos = GetID ();
                QueryExit whereTo = startMsg.get_whichOne ();
                whereTo.Print ();
                cout << " ";
                HoppingUpstreamMsg newMsg (curPos, whereTo, startMsg);
                SendHoppingUpstreamMsg (newMsg);
            }
            cerr << "\n";

        } else
            FATAL ("Why did I get a second IKillYouMessage?");
    } else if (message.get_message ().Type() == CentralHashMessage::type) {

        // remember the central hash table
        CentralHashMessage msg;
        msg.swap(message.get_message());
        centralHashTable.swap (msg.get_centralHash ());
        hashTableReady = true;


        FOREACH_TWL( qe, needToStart ) {
            SendStartProducingMsg(qe);
        } END_FOREACH

        needToStart.Clear();

    } else {
        FATAL ("Got a direct message that was not an IKillYouMessage\n");
    }
}

void JoinWayPointImp :: TypeSpecificConfigure (WayPointConfigureData &configData) {
    PDEBUG ("JoinWayPointImp :: TypeSpecificConfigure ()");

    JoinConfigureData tempConfig;
    tempConfig.swap (configData);

    if (state != FINE)
        FATAL ("Somehow, you are trying to configure a dead/dying join waypoint!\n");

    // first, extract all of the new queries that flow through this waypoint
    tempConfig.get_newFlowThruQueries ().MoveToStart ();
    stillGoing.MoveToFinish ();
    stillGoing.SwapRights (tempConfig.get_newFlowThruQueries ());

    // now, deal with all of the new queries that end at this waypoint
    QueryExitContainer &endingOnes = tempConfig.get_newEndingQueries ();
    for (endingOnes.MoveToStart (); endingOnes.RightLength (); endingOnes.Advance ()) {

        // get the meta data
        QueryExit tempExit = endingOnes.Current (), tempExitCopy = endingOnes.Current ();
        WayPointID myID = GetID (), myIDCopy = GetID ();

        // create the actual notification first
        if( hashTableReady ) {
            SendStartProducingMsg(tempExit);
        }
        else {
            needToStart.Append(tempExit);
        }
    }

    endingOnes.MoveToStart ();
    stillGoing.MoveToFinish ();
    stillGoing.SwapRights (endingOnes);

    // remember the identifier for the hash tbale cleaner
    hashTableCleaner = tempConfig.get_hashTableCleaner ();

    // set our special join waypoint ID
    WayPointID myID = GetID ();
    myJoinWayPointID = HashTableCleanerWayPointImp :: metaData.NewJoinWaypoint (
            myID, tempConfig.get_myDiskBasedTwinID ());

    if( !hashTableReady ) {
        GetHashTable ght(GetID());

        WayPointID cleanerID = hashTableCleaner;
        DirectMsg msg(cleanerID, ght);
        SendDirectMsg(msg);
    }
}

void JoinWayPointImp :: ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message) {
    PDEBUG ("JoinWayPointImp :: ProcessHoppingDownstreamMsg ()");

    // see if we have a query done message
    if (!strcmp (message.get_msg ().TypeName (), "QueryDoneMsg")) {

        // do the cast via a swap
        QueryDoneMsg temp;
        temp.swap (message.get_msg ());

        // we do two things here.  First, we note that this query is no longer running by taking
        // it out of the "stillGoing" set.  Second, we determine if it is a RHS query, and if it
        // is, we remember this
        for (temp.get_whichOnes ().MoveToStart (); temp.get_whichOnes ().RightLength (); temp.get_whichOnes ().Advance ()) {

            // first, see if this is one that we can remove from the "still going" set
            for (stillGoing.MoveToStart (); stillGoing.RightLength (); ) {
                if (temp.get_whichOnes ().Current ().IsEqual (stillGoing.Current ())) {
                    QueryExit tempExit;
                    stillGoing.Remove (tempExit);
                } else {
                    stillGoing.Advance ();
                }
            }

            // now, see if this query belongs to us
            if (temp.get_whichOnes ().Current ().exit == GetID ()) {

                // if it does, then remember it as being a completed RHS query
                QueryExit tempExit = temp.get_whichOnes ().Current ();
                cout << "The RHS for a query finished: ";
                tempExit.Print ();
                cout << "\n";

                // NOTE: doneRHS needs to be purged of queries when they are
                // ejected or else RHS will not restart when new queries are
                // added.
                doneRHS.Insert (tempExit);

            } else {

                QueryExit tempExit = temp.get_whichOnes ().Current ();
                cout << "The LHS for a query finished: ";
                tempExit.Print ();

            }

        }

        // if we are under normal operation, then we have to do all of the nexcessary book-keeping
        // to keep track of who has finished
        if (state == FINE) {

            // extract the queries that are done, and if they are "our" queries, process them
            for (temp.get_whichOnes ().MoveToStart (); temp.get_whichOnes ().RightLength (); ) {

                // if this is our query...
                if (temp.get_whichOnes ().Current ().exit == GetID ()) {

                    cout << "Got a match ";
                    temp.get_whichOnes ().Current ().Print ();
                    cout << "\n";

                    // remove it
                    QueryExit tempExit;
                    temp.get_whichOnes ().Remove (tempExit);

                    // there are two cases: either we have intercepted the LHS message matching this one, or not
                    // so we loop through all of the intercepted messages to try to find it
                    int foundAMate = 0;
                    for (waitingOnRHS.MoveToStart (); waitingOnRHS.RightLength (); waitingOnRHS.Advance ()) {

                        // if we have been supressing the LHS messahe, send it on
                        if (waitingOnRHS.Current ().get_whichOne ().query.IsEqual (tempExit.query)) {
                            foundAMate = 1;

                            // remove the waiting notification
                            StartProducingMsg startMsg;
                            waitingOnRHS.Remove (startMsg);

                            // and build the new hopping message that will start the LHS
                            WayPointID curPos = GetID ();
                            QueryExit whereTo = startMsg.get_whichOne ();

                            cout << "Sending to ";
                            cout << "asking ";
                            whereTo.Print ();
                            cout << " to start in response to a RHS finish.\n";
                            cout << "\n";
                            HoppingUpstreamMsg newMsg (curPos, whereTo, startMsg);
                            SendHoppingUpstreamMsg (newMsg);

                            break;
                        }
                    }

                    // in this case, we got a LHS query done message...
                } else {
                    temp.get_whichOnes ().Advance ();
                }
            }

            // here we loop through all of the queries that are done... for any that are LHS queries,
            // we will let the cleaner know that the corresponding RHS queries are now completed
            QueryExitContainer allComplete;
            for (temp.get_whichOnes ().MoveToStart (); temp.get_whichOnes ().RightLength ();
                    temp.get_whichOnes ().Advance ()) {

                // see if the current one is a LHS query
                if (!(temp.get_whichOnes ().Current ().exit == GetID ())) {

                    // if it is a LHS query, then we note that we can remove the correspnding RHS
                    QueryExit tempExit;
                    tempExit.exit = GetID ();
                    tempExit.query = temp.get_whichOnes ().Current ().query;
                    cout << "Sending ";
                    tempExit.Print ();
                    cout << "\n";
                    allComplete.Insert (tempExit);
                }
            }

            // now we actually send the messahe to the cleaner
            allComplete.MoveToStart ();
            if (allComplete.RightLength ()) {

                // we need to do two things.  First, we need to send a query done message to the cleaner
                // that contains the IDs for all of the LHS queries
                QueryDoneMsg someAreDone (GetID (), allComplete);

                // then set up and send the message letting the cleaner know these can be extracted
                WayPointID dest = hashTableCleaner;
                DirectMsg toCleaner (dest, someAreDone);
                SendDirectMsg (toCleaner);

            }

            // lastly, send out a query done message to all of the people down the graph
            temp.swap (message.get_msg ());
            SendHoppingDownstreamMsg (message);

        } else if (state == DYING) {

            // if we are dying and we find we have no more queries...
            stillGoing.MoveToStart ();
            if (!stillGoing.RightLength ()) {

                // let the cleaner know that we are dead
                WayPointID curPos = GetID ();
                cout << "This waypoint is dead: ";
                curPos.Print ();
                cout << "\n";
                WayPointDeadMsg deadMsg (curPos);
                WayPointID dest = hashTableCleaner;
                DirectMsg toCleaner (dest, deadMsg);
                SendDirectMsg (toCleaner);

                state = DEAD;
            }

        } else if (state == DEAD) {

            // if we are in here, then a dead waypoint somehow got a query done message, which makes
            // no sense at all
            FATAL ("How did a dead waypoint see a query done message?\n");
        }

    } else {

        // if we are way down here, we didn't get a query done message, so just send it on
        SendHoppingDownstreamMsg (message);
    }
}

void JoinWayPointImp :: DoneProducing (QueryExitContainer &whichOnes, HistoryList &history, int result, ExecEngineData &message) {
    PDEBUG ("JoinWayPointImp :: DoneProducing ()");

    // if the result is zero, it means we just processed a hashing operation
    if (result == 0) {

        // send an ack message
        SendAckMsg (whichOnes, history);

        // and then take a look and see if we got back any over-full segments
        JoinHashResult temp;
        temp.swap (message);
        temp.get_sampledQueries ().MoveToStart ();
        if (temp.get_sampledQueries ().RightLength ()) {

            // we did find an over-full segment...
            TooFullMessage outMsg (GetID (), temp.get_sampledQueries ());
            WayPointID dest = hashTableCleaner;
            DirectMsg toCleaner (dest, outMsg);
            SendDirectMsg (toCleaner);

        }

        // and make sure that no data will be sent out to the system by "emptying" the outgoing message
        ExecEngineData returnMe;
        returnMe.swap (message);
    }
}

static int counter = 0;
void JoinWayPointImp :: ProcessHoppingDataMsg (HoppingDataMsg &data) {

    PDEBUG ("JoinWayPointImp :: ProcessHoppingDataMsg ()");

    // at this point, we are ready to create the work spec.  First we figure out what queries
    // we are being asked to process
    QueryExitContainer whichOnes;
    whichOnes.copy (data.get_dest ());

    // now, we will count how many come from the LHS and how many from the RHS
    int numLHS = 0, numRHS = 0;
    QueryExitContainer flowThrus, endings;
    GetEndingQueryExits (endings);
    GetFlowThruQueryExits (flowThrus);
    for (whichOnes.MoveToStart (); whichOnes.RightLength (); whichOnes.Advance ()) {

        WayPointID myID;
        myID = GetID ();

        // see if this is a RHS query exit
        for (endings.MoveToStart (); endings.RightLength (); endings.Advance ()) {
            if (whichOnes.Current ().IsEqual (endings.Current ())) {
                numRHS++;
                //cerr << "In join RHS [";
                //myID.Print ();
                //cerr << "]: (";
                //whichOnes.Current ().Print ();
                //cerr << ") ";
            }
        }

        // see if this is a LHS query exit
        for (flowThrus.MoveToStart (); flowThrus.RightLength (); flowThrus.Advance ()) {
            if (whichOnes.Current ().IsEqual (flowThrus.Current ())) {
                numLHS++;
                //cerr << "In join LHS [";
                //myID.Print ();
                //cerr << "]: (";
                //whichOnes.Current ().Print ();
                //cerr << ") ";
            }
        }
    }

    if (numLHS > 0 && numRHS > 0)
        FATAL ("Got a chunk with both LHS and RHS queries in a join.\n");

    // get the chunk to process
    ChunkContainer temp;
    data.get_data ().swap (temp);


    // if this is a LHS one...
    if (numLHS > 0) {

        // there are two cases: either we are probing the LHS (normal operation) or we are hashing it
        // which is the case if our waypoint is wounded
        if (state == FINE) {

            // this is just a normal LHS probe, so we do not need to be nice
            GenericWorkToken returnVal;
            if (!RequestTokenImmediate (CPUWorkToken::type, returnVal)) {

                // if we do not get one, then we will just return a drop message to the sender
                SendDropMsg (data.get_dest (), data.get_lineage ());
                return;
            }
            CPUWorkToken myToken;
            myToken.swap (returnVal);

            // construct the work description...
            // clone our copy of the central hash table
            HashTable tempTable;
            tempTable.Clone (centralHashTable);
            JoinLHSWorkDescription workDesc (myJoinWayPointID, whichOnes, temp.get_myChunk (), tempTable);

            // and get the work done!
            WayPointID myID;
            myID = GetID ();

            WorkFunc myFunc;
            myFunc = GetWorkFunction (JoinLHSWorkFunc::type);

            myCPUWorkers.DoSomeWork (myID, data.get_lineage (), data.get_dest (), myToken, workDesc, myFunc);

        } else if (state == DYING) {

            // in this case, we are going to do a LHS hash insert, so we will request at a lower priority
            GenericWorkToken returnVal;
            if (!RequestTokenImmediate (CPUWorkToken::type, returnVal, 2)) {

                // if we do not get one, then we will just return a drop message to the sender
                SendDropMsg (data.get_dest (), data.get_lineage ());
                return;
            }
            CPUWorkToken myToken;
            myToken.swap (returnVal);

            // construct the work description...
            // clone our copy of the central hash table
            HashTable tempTable;
            tempTable.Clone (centralHashTable);
            JoinLHSHashWorkDescription workDesc (myJoinWayPointID, whichOnes, temp.get_myChunk (), tempTable);

            // and get the work done!
            WayPointID myID;
            myID = GetID ();

            WorkFunc myFunc;
            myFunc = GetWorkFunction (JoinLHSHashWorkFunc::type);

            counter++;
            // cout << "Got " << counter << " LHS chunks from the dying waypoint.\n";
            myCPUWorkers.DoSomeWork (myID, data.get_lineage (), data.get_dest (), myToken, workDesc, myFunc);
        } else {
            for (whichOnes.MoveToStart (); whichOnes.RightLength (); whichOnes.Advance ()) {
                whichOnes.Current ().Print ();
            }
            WayPointID myID;
            myID = GetID ();
            cout << "I am:\n";
            myID.Print ();
            FATAL ("How did I get a LHS chunk for a dead waypoint?");
        }

    } else if (numRHS > 0) {

        // in this case, we are going to do a RHS hash insert, so we will request at a lower priority
        GenericWorkToken returnVal;
        if (!RequestTokenImmediate (CPUWorkToken::type, returnVal, 2)) {

            // if we do not get one, then we will just return a drop message to the sender
            SendDropMsg (data.get_dest (), data.get_lineage ());
            return;
        }
        CPUWorkToken myToken;
        myToken.swap (returnVal);

        // construct the work description...
        // clone our copy of the central hash table
        HashTable tempTable;
        tempTable.Clone (centralHashTable);
        JoinRHSWorkDescription workDesc (myJoinWayPointID, whichOnes, temp.get_myChunk (), tempTable);

        // and get the work done!
        WayPointID myID;
        myID = GetID ();
        WorkFunc myFunc = GetWorkFunction (JoinRHSWorkFunc::type);
        myCPUWorkers.DoSomeWork (myID, data.get_lineage (), data.get_dest (), myToken, workDesc, myFunc);

    } else {
        FATAL ("Why did I get a chunk with no querys for the join to process!?!");
    }
}

void JoinWayPointImp :: ProcessHoppingUpstreamMsg (HoppingUpstreamMsg &message) {
    PDEBUG ("JoinWayPointImp :: ProcessHoppingUpstreamMsg ()");

    // if we did not get a start producing message, just send it on
    if (message.get_msg ().Type()!= StartProducingMsg::type) {
        SendHoppingUpstreamMsg (message);
        return;
    }

    // if we got a start producing message, then make sure the query goes through us
    StartProducingMsg myMessage;
    myMessage.swap (message.get_msg ());
    // message.get_msg () is junk now

    QueryExit &queryToStart = myMessage.get_whichOne ();
    cerr << "Asked to start: ";
    queryToStart.Print ();
    cerr << " in the join.\n";
    QueryExitContainer putResHere;
    GetFlowThruQueryExits (putResHere);
    int gotIt = 0;
    for (putResHere.MoveToStart (); putResHere.RightLength (); putResHere.Advance ()) {
        if (putResHere.Current ().IsEqual (queryToStart))
            gotIt = 1;
    }

    if (gotIt == 0) {
        cout << "Strange, a join intercepted a start producing message that for a query it doesn't know!\n";
        return;
    }

    // at this point, it does go through us.  So see if it's matching RHS is done
    int forwardItOn = 0;
    for (doneRHS.MoveToStart (); doneRHS.RightLength (); doneRHS.Advance ()) {

        // see if we found the match
        if (doneRHS.Current ().query == queryToStart.query) {
            QueryExit temp = doneRHS.Current();
            forwardItOn = 1;
            break;
        }
    }

    // we need to forward it on if the RHS of the query is done, or if we are not running normally (in the latter case, we
    // start up the query even if we have not finished the LHS)
    if (forwardItOn || state != FINE) {

        // put myMessage back in message
        myMessage.swap (message.get_msg ());

        // then we just send the start request on directly
        SendHoppingUpstreamMsg (message);
        cout << "and we forwarded it on.\n";
        return;
    }

    // if we made it here, we will just hold this one for future use
    cout << "but we held it.\n";
    waitingOnRHS.Insert (myMessage);
    waitingOnRHS.MoveToStart ();
    cout << "are " << waitingOnRHS.RightLength () << " queries held.\n";

    PDEBUG ("JoinWayPointImp :: ProcessHoppingUpstreamMsg - finished ()");
}

