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

#include "AggWayPointImp.h"
#include "CPUWorkerPool.h"
#include "Logging.h"

using namespace std;

AggWayPointImp :: AggWayPointImp () {
	PDEBUG ("AggWayPointImp :: AggWayPointImp ()");
}

AggWayPointImp :: ~AggWayPointImp () {
	PDEBUG ("AggWayPointImp :: AggWayPointImp ()");
}

void AggWayPointImp :: DoneProducing (QueryExitContainer &whichOnes, HistoryList &history,
																			int result, ExecEngineData& data) {
	PDEBUG ("AggWayPointImp :: DoneProducing ()");
	// we do not touch data

	// if the result is zero, it means we just aggregated a chunk
	if (result == 0) {
		SendAckMsg (whichOnes, history);
	}
}

void AggWayPointImp :: PrintMyAggs(void){
  cout << "myMAP: ";
  myAggs.Lock();
  for (myAggs.MoveToStart(); !myAggs.AtEnd(); myAggs.Advance()){
    myAggs.CurrentKey().Print();
    cout << "\t" << myAggs.CurrentData().GetContent() << "\t\t";
  }
  myAggs.Unlock();
  cout << endl;

}

void AggWayPointImp :: RequestGranted (GenericWorkToken &returnVal) {
	PDEBUG ("AggWayPointImp :: RequestGranted ()");

	// first remove from queriesToComplete any queries in queriesCompleted
	// if queriesToComplete is empty, return the token and do nothing

	FOREACH_TWL(qe, queriesToComplete){
	  if (qe.query.Overlaps(queriesCompleted)){
	    // remove qe
	    queriesToComplete.Remove(qe);
	  } else {
	    // add qe.query to queriesCompleted;
	    queriesCompleted.Union(qe.query);
	  }
	}END_FOREACH


	// we know that the reason that this request is granted is that we have one or more
	// query exits that we are ready to finish up... first, cast the return val appropriately
	CPUWorkToken myToken;
	myToken.swap (returnVal);

	if (queriesToComplete.Length() == 0) {
		GiveBackToken(myToken);
		return;
	}
	// at this point, we are ready to create the work spec... copy the set of queries to complete,
	// since they are going to have to go to both the work description, the lineage, and the work request
	QueryExitContainer whichOnes;
	whichOnes.copy (queriesToComplete);

	AggStorageMap copyMap;
	copyMap.copy(myAggs);
	FinishAggregate workDesc (whichOnes, copyMap);

	// we now create a history list data object...
	AggHistory temp (GetID ());
	HistoryList tempList;
	tempList.Insert (temp);

	//cout << "Final Compuptation\t -- ";
	//PrintMyAggs();

	// now, actually get the work done!
	WayPointID myID;
	myID = GetID ();
	QueryExitContainer whichOnes2;
	whichOnes2.copy (queriesToComplete);
	WorkFunc myFunc = GetWorkFunction (AggFinishUpWorkFunc::type);
	myCPUWorkers.DoSomeWork (myID, tempList, whichOnes2, myToken, workDesc, myFunc);
}

void AggWayPointImp :: ProcessHoppingDataMsg (HoppingDataMsg &data) {
	PDEBUG ("AggWayPointImp :: ProcessHoppingDataMsg ()");

	// in this case, the first thing we do is to request a work token
	GenericWorkToken returnVal;
	if (!RequestTokenImmediate (CPUWorkToken::type, returnVal)) {

		// if we do not get one, then we will just return a drop message to the sender
		SendDropMsg (data.get_dest (), data.get_lineage ());
		return;
	}

	// convert the token into the correct type
	CPUWorkToken myToken;
	myToken.swap (returnVal);

	// OK, got a token!  So first thing is to extract the chunk from the message
	ChunkContainer temp;
	data.get_data ().swap (temp);

	// at this point, we are ready to create the work spec.  First we figure out what queries to finish up
	QueryExitContainer whichOnes;
	whichOnes.copy (data.get_dest ());
	whichOnes.MoveToStart ();

	// now build the work spec
	AggStorageMap copyMap;
	copyMap.copy(myAggs);
	AggregateOneChunk workDesc (whichOnes, copyMap, temp.get_myChunk ());

	// if we have a chunk produced by a table waypoint log it
	data.get_lineage().MoveToFinish ();
	data.get_lineage().Retreat ();
	if (CHECK_DATA_TYPE(data.get_lineage().Current(), TableHistory)){
		TableHistory hLin;
		hLin.swap(data.get_lineage().Current());

		ChunkID& id = hLin.get_whichChunk();

		TableScanInfo infoTS;
		id.getInfo(infoTS);

		LOG_ENTRY_P(2, "CHUNK %d of %s Processed by Aggregate",
								id.GetInt(), infoTS.getName().c_str()) ;

		// put it back;
		hLin.swap(data.get_lineage().Current());
	}

	//PrintMyAggs();

	// now, actually get the work done!
	WayPointID myID;
	myID = GetID ();
	WorkFunc myFunc = GetWorkFunction (AggOneChunkWorkFunc::type);
	myCPUWorkers.DoSomeWork (myID, data.get_lineage (), data.get_dest (), myToken, workDesc, myFunc);
}

// the only kind of message we are interested in is a query done message... everything else is
// just forwarded on, down the graph
void AggWayPointImp :: ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message) {
	PDEBUG ("AggWayPointImp :: ProcessHoppingDownstreamMsg ()");

	// this is the set of queries that we are waiting to finish up
	queriesToComplete.MoveToStart ();
	int alreadyAreSome = queriesToComplete.RightLength ();

	// see if we have a query done message
	if (CHECK_DATA_TYPE (message.get_msg (), QueryDoneMsg)) {

		// do the cast via a swap
		QueryDoneMsg temp;
		temp.swap (message.get_msg ());

		// extract the queries that are done, add them to the list of those to complete
		for (temp.get_whichOnes ().MoveToStart (); temp.get_whichOnes ().RightLength (); ) {
			QueryExit myExit;
			temp.get_whichOnes ().Remove (myExit);
			queriesToComplete.Insert (myExit);
		}

		// ask for a worker, if we have not already asked
		if (!alreadyAreSome) {
			RequestTokenDelayOK (CPUWorkToken::type);
		}

	} else {

		SendHoppingDownstreamMsg (message);
	}
}

void AggWayPointImp :: ProcessAckMsg (QueryExitContainer &whichOnes, HistoryList &lineage) {
	PDEBUG ("AggWayPointImp :: ProcessAckMsg ()");

	// make sure that the HistoryList has one item that is of the right type
	lineage.MoveToStart ();
	FATALIF (lineage.RightLength () != 1 || !CHECK_DATA_TYPE(lineage.Current (), AggHistory),
		"Got a bad lineage item in an ack to an agg waypoint!");

	/** Alin: This should not be done here but in the AggFishish. This removes
	    the aggregate before it is processed, possibly.



	// go ahead and remove this set of query exits from the agg storage
	for (whichOnes.MoveToStart (); whichOnes.RightLength (); whichOnes.Advance ()) {

		// remove this particular query exit from the agg storage
		QueryID tempExit;
		AggStorage removeMe;
		myAggs.Lock ();
		myAggs.Remove (whichOnes.Current ().query, tempExit, removeMe);
		myAggs.Unlock ();
	}

	*/

	// now, send a "done" message onward
	QueryExitContainer allCompleteCopy;
	allCompleteCopy.copy (whichOnes);
	QueryDoneMsg someAreDone (GetID (), whichOnes);
	HoppingDownstreamMsg myOutMsg (GetID (), allCompleteCopy, someAreDone);
	SendHoppingDownstreamMsg (myOutMsg);

}

void AggWayPointImp :: ProcessDropMsg (QueryExitContainer &whichOnes, HistoryList &lineage) {
	PDEBUG ("AggWayPointImp :: ProcessDropMsg ()");

	// make sure that the HistoryList has one item that is of the right type
	lineage.MoveToStart ();
	FATALIF (lineage.RightLength () != 1 || !CHECK_DATA_TYPE (lineage.Current (), AggHistory),
		"Got a bad lineage item in a drop sent to an agg waypoint!");

	// this is the set of queries that we are waiting to finish up
	queriesToComplete.MoveToStart ();
	int alreadyAreSome = queriesToComplete.RightLength ();

	// extract the queries that are done, add them to the list of those to complete
	for (whichOnes.MoveToStart (); whichOnes.RightLength (); ) {
		QueryExit myExit;
		whichOnes.Remove (myExit);
		queriesToComplete.Insert (myExit);
	}

	// ask for a worker to re-send the chunk, if we have not already asked
	if (!alreadyAreSome)
		RequestTokenDelayOK (CPUWorkToken::type);
}

