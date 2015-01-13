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

// FIXME: Obsolete. Remove.

#include "TableScanWayPointImp.h"
#include "CPUWorkerPool.h"

using namespace std;

TableScanWayPointImp :: TableScanWayPointImp () {
	numDone = 0;
	allDone = 0;
	lastOne = -1;
	numRequestsOut = 0;
}

TableScanWayPointImp :: ~TableScanWayPointImp () {
	if (numDone != 0) {
		delete [] numDone;
		for (int i = 0; i < numQueryExits; i++) {
			delete [] allDone[i];
		}
		delete [] allDone;
	}
}

void TableScanWayPointImp :: TypeSpecificConfigure (WayPointConfigureData &configData) {

	// first, extract the extra config info
	TableScanConfigureData tempConfig;
	tempConfig.swap (configData);

	// set up the data producers
	myExits.swap (tempConfig.get_myExits ());
	myExits.MoveToStart ();
	numQueryExits = myExits.RightLength ();

	// note if we are LHS or RHS
	isLHS = tempConfig.get_isLHS ();

	// numDone is the number that have actually been acked
	numDone = new int[numQueryExits];

	// this is a set of arrays that tell us what chunks have been sent
	allDone = new int *[numQueryExits];

	// init everything... to being with, there are no unsent chunks for any query
	for (int i = 0; i < numQueryExits; i++) {
		allDone[i] = new int[5000];
		for (int j = 0; j < 5000; j++) {
			allDone[i][j] = 1;
			numDone[i] = 0;
		}
	}

}

void TableScanWayPointImp :: RequestGranted (GenericWorkToken &returnVal) {

	// the reason that this request was granted is that we had previously asked for a token that would
	// allow us to go off and produce a new chunk.  Note that in this toy version of the TableScanWayPointImp
	// the table scanner actually uses a CPU worker to produce its chunks, and it actually sends a function
	// off to a CPU to get the chunk produced.  In a "real" version of this waypoint, we would presumably
	// be requesting a disk token, and once we had it we would be asking a diferent resource (not the CPU
	// manager) to produce the chunk
	DiskWorkToken myToken;
	myToken.swap (returnVal);

	// note that we have one less request out
	numRequestsOut--;

	// this is the set of query exits involved
	QueryExitContainer myOutputExits;
	int gotOne = 0;
	for (int i = lastOne + 1; i <= lastOne + 5000; i++) {

		// see if one of the exits needs this block
		myExits.MoveToStart ();
		for (int j = 0; myExits.RightLength (); j++, myExits.Advance ()) {

			// if we got an exit that needs this block, then record it
			if (allDone[j][i % 5000] == 0) {
				allDone[j][i % 5000] = 1;
				gotOne = 1;
				lastOne = i % 5000;
				QueryExit myExit = myExits.Current ();
				myOutputExits.Insert (myExit);
			}
		}

		if (gotOne) {
			break;
		}
	}

	// if three are no query exits that need data, then just give back the token and get out
	if (!gotOne) {
		GiveBackToken (myToken);
		return;
	}

	// set up the lineage... since the chunk originates here, we create the object from scratch
	QueryExitContainer myOutputExitsCopy, myOutputExitsCopyTwo;
	myOutputExitsCopy.copy (myOutputExits);
	myOutputExitsCopyTwo.copy (myOutputExits);
	TableScanHistory myHistory (GetID (), lastOne, myOutputExits);
	HistoryList tempList;
	tempList.Insert (myHistory);

	// set up the work description
	TableScanWorkDescription workDesc (lastOne, isLHS, myOutputExitsCopy);

	// now, actually get the chunk sent out!  Again, note that here we use a CPU to do this...
	// but that is just because we have a toy table scan imp
	WorkFunc myFunc = GetWorkFunction (TableScanWorkFunc::type);
	WayPointID tempID = GetID ();
	myDiskWorkers.DoSomeWork (tempID, tempList, myOutputExitsCopyTwo, myToken, workDesc, myFunc);

	// queue up some more work requests
	for (; numRequestsOut < 5; numRequestsOut++) {
		RequestTokenDelayOK (DiskWorkToken::type);
	}
}

void TableScanWayPointImp :: ProcessHoppingUpstreamMsg (HoppingUpstreamMsg &message) {

	FATALIF (!CHECK_DATA_TYPE (message.get_msg (), StartProducingMsg),
		"Strange, why did a table scan get a HUS of a type that was not 'Start Producing'?");

	// access the content of the message
	StartProducingMsg myMessage;
	message.get_msg ().swap (myMessage);

	// see what query we are asked to start
	QueryExit &queryToStart = myMessage.get_whichOne ();
	cout << "About to start ";
	queryToStart.Print ();
	cout << endl;

	// start up the particular query that is specificied in the message
	myExits.MoveToStart ();
	for (int i = 0; i < numQueryExits; i++, myExits.Advance ()) {

		// see if this one is the query we are asked to start up
		if (myExits.Current ().IsEqual (queryToStart)) {
			for (int j = 0; j < 5000; j++)
				allDone[i][j] = 0;
			break;
		}
	}

	// if we don't have five requests for tokens out (five is a random choice) then send them
	for (; numRequestsOut < 5; numRequestsOut++) {
		RequestTokenDelayOK (DiskWorkToken::type);
	}
}

void TableScanWayPointImp :: ProcessDropMsg (QueryExitContainer &whichExits, HistoryList &lineage) {

	// make sure that the HistoryList has one item that is of the right type
	lineage.MoveToStart ();
	FATALIF (lineage.RightLength () != 1 || !CHECK_DATA_TYPE (lineage.Current (), TableScanHistory),
		"Got a bad lineage item in an ack to a table scan waypoint!");

	// get the history out
	TableScanHistory myHistory;
	lineage.Remove (myHistory);

	// now go and un-set the done bits for everyone who was dropped
	myExits.MoveToStart ();
	for (int i = 0; i < numQueryExits; i++, myExits.Advance ()) {
		for (whichExits.MoveToStart (); whichExits.RightLength (); whichExits.Advance ()) {

			// if this exit was one dropped, then reset
			if (whichExits.Current ().IsEqual (myExits.Current ())) {
				if (allDone[i][myHistory.get_whichChunk ()] == 0) {
					FATAL ("Dropping a chunk not sent.\n");
				}
				allDone[i][myHistory.get_whichChunk ()] = 0;
			}
		}
	}

	// if we don't have five requests for tokens out (five is a random choice) then send them
	for (; numRequestsOut < 5; numRequestsOut++) {
		RequestTokenDelayOK (DiskWorkToken::type);
	}
}


void TableScanWayPointImp :: ProcessAckMsg (QueryExitContainer &whichExits, HistoryList &lineage) {

	// make sure that the HistoryList has one item that is of the right type
	lineage.MoveToStart ();
	FATALIF (lineage.RightLength () != 1 || !CHECK_DATA_TYPE (lineage.Current (), TableScanHistory),
		"Got a bad lineage item in an ack to a table scan waypoint!");

	// get the history out
	TableScanHistory myHistory;
	lineage.Remove (myHistory);

	// this is the set of totally completed queries
	QueryExitContainer allComplete;

	// increment the count of done chunks for everyone
	myExits.MoveToStart ();
	for (int i = 0; i < numQueryExits; i++, myExits.Advance ()) {
		for (whichExits.MoveToStart (); whichExits.RightLength (); whichExits.Advance ()) {

			// if this exit was one dropped, then reset
			if (whichExits.Current ().IsEqual (myExits.Current ())) {
				numDone[i]++;
				if (allDone[i][myHistory.get_whichChunk ()] == 0) {
					FATAL ("Acking a chunk not sent.\n");
				}
				if (numDone[i] == 5000) {
					QueryExit temp;
					temp = whichExits.Current ();
					cout << "Complete: ";
					temp.Print ();
					allComplete.Insert (temp);

					cout << "\n";
					for (int j = 0; j < numQueryExits; j++) {
						cout << numDone[j] << " ";
						int cnt = 0;
						for (int k = 0; k < 5000; k++) {
							if (allDone[j][k])
								cnt++;
						}
						cout << " (" << cnt << ") ";
					}
					cout << "\n";
				}
			}
		}
	}

	// if anyone is done, send the notification
	if (allComplete.RightLength ()) {
		QueryExitContainer allCompleteCopy;
		allCompleteCopy.copy (allComplete);
		QueryDoneMsg someAreDone (GetID (), allComplete);
		HoppingDownstreamMsg myOutMsg (GetID (), allCompleteCopy, someAreDone);
		SendHoppingDownstreamMsg (myOutMsg);
	}

	// if we don't have five requests for tokens out (five is a random choice) then send them
	for (; numRequestsOut < 5; numRequestsOut++) {
		RequestTokenDelayOK (DiskWorkToken::type);
	}
}
