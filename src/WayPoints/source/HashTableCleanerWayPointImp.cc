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

#include "Constants.h"
#include "WayPointImp.h"
#include "HashTableCleanerWayPointImp.h"
#include "CPUWorkerPool.h"
#include "HashTableMacros.h"
#include "ExecEngineData.h"
#include "PriorityList.h"
#include "Logging.h"
#include "Properties.h"
#include "Constants.h"

using namespace std;

HashTableCleanerManager HashTableCleanerWayPointImp :: metaData;

HashTableCleanerWayPointImp :: HashTableCleanerWayPointImp () {
    numWorkers = 0;
}

HashTableCleanerWayPointImp :: ~HashTableCleanerWayPointImp () {}

void HashTableCleanerWayPointImp :: BuildCentralHash(){
    LOG_ENTRY(1, "Building Central Hash");
    centralHashTable.Allocate(NUM_EXEC_ENGINE_THREADS);
    HashTable hCopy;
    hCopy.copy(centralHashTable);
    metaData.AddCentralHashTable (hCopy);
    centralHashBuilt = true;
}

void HashTableCleanerWayPointImp :: ProcessAckMsg (QueryExitContainer &whichOnes, HistoryList &lineage) {

    PDEBUG("HashTableCleanerWayPointImp :: ProcessAckMsg");
    // verify that there is only one history object
    lineage.MoveToStart ();
    if (lineage.RightLength () != 1)
        FATAL ("Hash table cleaner didn't get exactly one history object back!?!?!");

    HashCleanerHistory tempHistory;
    tempHistory.swap (lineage.Current ());

    // get the query exit that was acked
    whichOnes.MoveToStart ();
    if (lineage.RightLength () != 1)
        FATAL ("Hash table got ack from more than a single table writing waypoint!?!?!");

    //  cout << "Got an ack message in the cleaner.\n";

    // note the ack, and see if this waypoint is now totally done... if it is, then send a direct
    // message that notes this fact
    WayPointID whichOne = whichOnes.Current ().exit;
    QueryExitContainer completedQueries;
    metaData.GotAck (whichOne, tempHistory.get_whichSegment (), completedQueries);
    SendDone (completedQueries);

    // and if we need to, put some work requests out there
    SendOutRequests ();
}

void HashTableCleanerWayPointImp :: SendDone (QueryExitContainer &completedQueries) {

    for (completedQueries.MoveToStart (); completedQueries.RightLength (); ) {

        // set up the query done message... get the sender
        WayPointID sender = GetID ();

        // put the receiver into a list
        QueryExitContainer addressToDeliverTo;
        QueryExit temp = completedQueries.Current ();
        cout << "SendDone: ";
        temp.Print ();
        cout << "\n";
        addressToDeliverTo.Insert (temp);

        // now, create the list of completed queries
        QueryExitContainer theOneToDeliver;
        completedQueries.Remove (temp);

        theOneToDeliver.Insert (temp);
        // set up the done message
        QueryDoneMsg doneMsg (sender, theOneToDeliver);

        // set up the hopping downstream message
        HoppingDownstreamMsg downstreamMsg (sender, addressToDeliverTo, doneMsg);

        SendHoppingDownstreamMsg (downstreamMsg);
    }
}

void HashTableCleanerWayPointImp :: SendOutRequests () {

    // the idea here is to first suck up any available tokens... note that RequestGranted
    // will recursively call SendOutRequests, so we'll recursively use up all of the
    // available CPU tokens.  The recursion stops when there are no available tokens
    // (that is, when the request is not granted)... in that case, we send out a set of
    // delayed requests
    //
    // so... start out by asking for an available token

    // if one is not here, put out some delayed requests
    for (; numWorkers < MAX_CLEANER_CPU_WORKERS; numWorkers++) {
        //		cout << "Sending request\n";
      RequestTokenNowDelayOK (CPUWorkToken::type, 1);
    }
}

void HashTableCleanerWayPointImp :: ProcessDropMsg (QueryExitContainer &whichOnes, HistoryList &lineage) {

    //	cout << "Received a drop!\n";

    // verify that there is only one history object
    lineage.MoveToStart ();
    if (lineage.RightLength () != 1)
        FATAL ("Hash table cleaner didn't get exactly one history object back!?!?!");

    HashCleanerHistory tempHistory;
    tempHistory.swap (lineage.Current ());

    // get the query exit that was acked
    whichOnes.MoveToStart ();
    if (lineage.RightLength () != 1)
        FATAL ("Hash table got drop from more than a single table writing waypoint!?!?!");

    // note the drop, and see if this waypoint is now totally done
    WayPointID whichOne = whichOnes.Current ().exit;
    metaData.GotDrop (whichOne, tempHistory.get_whichSegment ());

    // Put some token requests out there
    SendOutRequests ();
}

// actually send some segment cleaning work out
void HashTableCleanerWayPointImp :: RequestGranted (GenericWorkToken &returnVal) {

    PDEBUG("HashTableCleanerWayPointImp :: RequestGranted");

    CPUWorkToken myToken;
    myToken.swap (returnVal);

    // now get all of the info we need to go out and process a hash table segment
    JoinWayPointIDList removeTheseWPsAndSend;
    JoinWayPointIDList removeTheseWPsAndHold;
    QueryExitContainer theseQueriesAreDone;
    JoinWayPointIDEquivalences equivalences;
    int whichSegment = metaData.GetOneToRebuild (removeTheseWPsAndSend, removeTheseWPsAndHold, theseQueriesAreDone, equivalences);

    // if no one had any work to do, give all of the tokens back and get outta here
    if (whichSegment == -1) {

        // give back the CPU token
        numWorkers--;
        GiveBackToken (myToken);

        // set the CPU priority cutoff so that anyone can run... this will unfreeze the inserts into the hash table
        SetPriorityCutoff (CPUWorkToken :: type, 999);

        // get out
        return;
    }

    //	cout << "About to clean segemnt " << whichSegment << "\n";

    // figure out where the result of the work needs to be sent
    QueryExitContainer myOutputExits;
    for (removeTheseWPsAndSend.MoveToStart (); removeTheseWPsAndSend.RightLength (); removeTheseWPsAndSend.Advance ()) {

        // now, find the matching disk waypoint
        for (equivalences.MoveToStart (); equivalences.RightLength (); equivalences.Advance ()) {

            // if we found it, remember it
            if (removeTheseWPsAndSend.Current () == equivalences.Current ().joinWayPointID) {
                QueryExit temp;
                temp.exit = equivalences.Current ().diskBasedTwinID;
                temp.query.Empty ();
                myOutputExits.Insert (temp);
                break;
            }
        }
    }

    DiskWorkTokenQueue useTheseTokens;

    // set up the history
    WayPointID myID = GetID ();
    HashCleanerHistory myHistory (myID, whichSegment);
    HistoryList tempList;
    tempList.Insert (myHistory);

    // get a copy of the hash table
    HashTable tempTable;
    tempTable.Clone (centralHashTable);

    // set up the work description
    HashCleanerWorkDescription workDesc (whichSegment, tempTable, useTheseTokens, removeTheseWPsAndSend,
            removeTheseWPsAndHold, theseQueriesAreDone, equivalences);

    // now, get the work sent out!
    WorkFunc myFunc = GetWorkFunction (CleanerWorkFunc::type);
    WayPointID tempID = GetID ();
    myCPUWorkers.DoSomeWork (tempID, tempList, myOutputExits, myToken, workDesc, myFunc);
}


// these comparison funcs are used in the next routine
int CompareFirst (const void *a, const void *b) {
    return ( *(unsigned int*)b - *(unsigned int*)a );
}

int CompareSecond (const void *a, const void *b) {
    return ( ((unsigned int*)a)[1] - ((unsigned int*)b)[1] );
}

void HashTableCleanerWayPointImp :: ProcessDirectMsg (DirectMsg &message) {

    PDEBUG("HashTableCleanerWayPointImp :: ProcessDirectMsg");

    if (!strcmp (message.get_message ().TypeName (), "GetHashTable")) {
        if (!centralHashBuilt)
            BuildCentralHash();

        HashTable centralHashCopy;
        centralHashCopy.Clone(centralHashTable);
        CentralHashMessage hashMessage(GetID(), centralHashCopy);
        DirectMsg centralHashMsg (message.get_message().get_sender(), hashMessage);
        SendDirectMsg (centralHashMsg);

    } else if (!strcmp (message.get_message ().TypeName (), "QueryDoneMsg")) {
    // in this case, we have been notified that a particular, non disk-based query/exit has completed


        // just extract the completed queries
        QueryDoneMsg myMsg;
        myMsg.swap (message.get_message ());
        metaData.AddCompletedQueries (myMsg.get_whichOnes ());

        // in this case, a waounded waypoint has now finally died
    } else if (!strcmp (message.get_message ().TypeName (), "WayPointDeadMsg")) {

        // note that this guy has actually died
        WayPointDeadMsg myMsg;
        myMsg.swap (message.get_message ());
        cout << "Now he is dead.";
        myMsg.get_sender ().Print ();
        cout << "\n";

        metaData.IsNowDead (myMsg.get_sender ());

        LOG_ENTRY(2, "Cleaner has started now");

        // set a higher priority cutoff for CPU token requests, so that anyone who wants
        // to write to the hash tbale will be blocked
        //SetPriorityCutoff (CPUWorkToken :: type, 1);

        // in this case, we have been notified that a particular hash table segment is too full; in
        // this case, we are passed a set of QueryExit, JoinWayPointID pairs; these are the ones
        // that were sampled from the hash table
    } else if (!strcmp (message.get_message ().TypeName (), "TooFullMessage")) {


        LOG_ENTRY(2, "Cleaner has started now");

        TooFullMessage myMsg;
        myMsg.swap (message.get_message ());

        // get the list of samples
        HashSegmentSample mySample;
        mySample.swap (myMsg.get_whatWeFound ());

        // now we have to do some counting... goal is to figure out which waypoints to kill (if any) so that
        // we can empty out the hash segment to an acceptable degree
        mySample.MoveToStart ();
        int len = mySample.RightLength ();

        // set a higher priority cutoff for CPU token requests, so that anyone who wants
        // to write to the hash tbale will be blocked
        //SetPriorityCutoff (CPUWorkToken :: type, 1);

        unsigned int counts[len * 2];

        // this tells us how many slots have been discarded as not REALLY being full becuase
        // they have data that would be emptied out anyway
        int notReallyFull = 0;

        for (int i = 0; mySample.RightLength (); mySample.Advance (), i += 2) {

            // see if this is a waypoint/query combo that is actually running
            if (metaData.IsActive (mySample.Current ().get_whichQueries (), mySample.Current ().get_whichWayPoint ())) {
                counts[i] = 1;
            } else {
                counts[i] = 0;
                notReallyFull++;
            }

            counts[i + 1] = mySample.Current ().get_whichWayPoint ();
        }

        // sort them bsaed upon the waypoints...
        qsort (counts, len, sizeof (unsigned int) * 2, CompareSecond);

        // and tally them up
        for (int i = 0; i < len * 2; ) {
            int j = i + 2;
            for (; j < len * 2 && counts[j + 1] == counts[i + 1]; j += 2) {
                counts[i] += counts[j];
                counts[j] = 0;
            }
            cout << "count was " << counts[i] << " for waypoint " << counts[i + 1] << "\n";
            i = j;
        }

        // now sort them again based upon the counts...
        qsort (counts, len, sizeof (unsigned int) * 2, CompareFirst);
        cout << "After sorting...";

        // now, loop through the waypoints once again and kill the worst offenders
        for (int i = 0; len - notReallyFull > len * FRAC_TO_TAKE_IT_DOWN; i += 2) {

            cout << "**count was " << counts[i] << " for waypoint " << counts[i + 1] << "\n";

            // note the fact that the waypoint is now dead
            JoinWayPointID tempID = counts[i + 1];
            metaData.IsNowDying (tempID);
            tempID = counts[i + 1];
            notReallyFull += counts[i];

            // tell him that he is dead
            WayPointID myID = GetID ();
            IKillYouMessage diediedie (myID);
            WayPointID oneToKill;
            metaData.LookUpRealWaypointID (oneToKill, tempID);
            cout << "killing ";
            oneToKill.Print ();
            DirectMsg dieMessage (oneToKill, diediedie);
            SendDirectMsg (dieMessage);
        }
    }

    // and if we need to, put some work requests out there
    SendOutRequests ();
}

void HashTableCleanerWayPointImp :: TypeSpecificConfigure (WayPointConfigureData &configData) {

    PDEBUG("HashTableCleanerWayPointImp :: TypeSpecificConfigure");
    // just remember the central hash table
    HashTableCleanerConfigureData tempConfig;
    tempConfig.swap (configData);
}

void HashTableCleanerWayPointImp :: DoneProducing (QueryExitContainer &whichOnes, HistoryList &history,
        int result, ExecEngineData &dataProduced) {

    PDEBUG("HashTableCleanerWayPointImp :: DoneProducing");

    // first, pull the actual result out
    ExtractionResult myResult;
    myResult.swap (dataProduced);

    // create a temporary result
    ExtractionContainer finalOutput;

    // extract the actual result
    metaData.AllDone (myResult, finalOutput);

    // there are two cases... it might be that we got back an empty result (that is, NO chunks
    // were extracted from the hash table segment).  In that case, we ask the meta-data object
    // to take care of the result of the cleaning right away, and we will give back a generic
    // ExecEngineData object (which prevents the exec engine from actually sending the result
    // anywhere)
    finalOutput.get_result ().MoveToStart ();
    if (!finalOutput.get_result ().RightLength ()) {
        QueryExitContainer theseAreDone;
        metaData.ProcessEmptyResult (finalOutput.get_whichSegment (), theseAreDone);

        // if we got any completed queries, then tell the relevant writer
        theseAreDone.MoveToStart ();
        if (theseAreDone.RightLength ()) {
            SendDone (theseAreDone);
        }

        // send back the empty result
        ExecEngineData generic;
        generic.swap (dataProduced);
        return;
    }

    // and send it on
    finalOutput.swap (dataProduced);

    // lastly, relaim the token and do some more cleaning, if needed
    GenericWorkToken putResHere;
    ReclaimToken (putResHere);
    RequestGranted (putResHere);
}

void HashTableCleanerWayPointImp :: ProcessHoppingUpstreamMsg (HoppingUpstreamMsg &message) {

    FATAL ("Error!  Why is anyone sending the hash cleaner a hopping upstram message?\n");

}

