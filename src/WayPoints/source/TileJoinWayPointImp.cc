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
#include "TileJoinWayPointImp.h"
#include "Constants.h"
#include "DiskPool.h"
#include "QueryManager.h"
#include "Logging.h"
#include "CPUWorkerPool.h"
#include "BStringIterator.h"
#include "BStringIterator.h"
#include "ColumnIterator.h"

using namespace std;

TileJoinWayPointImp :: TileJoinWayPointImp () {
	numCPURequestsOut = 0;
	numDiskRequestsOut = 0;
	numChunksLHS = 0;
	numChunksRHS = 0;
	number128 = 0;
	TOTALLHS = 0;
	TOTALRHS = 0;
}

TileJoinWayPointImp :: ~TileJoinWayPointImp () {
}

void TileJoinWayPointImp :: TypeSpecificConfigure (WayPointConfigureData &configData) {

	PDEBUG("TileJoinWayPointImp :: TypeSpecificConfigure");
	// first, extract the extra config info
	TileJoinConfigureData tempConfig;
	tempConfig.swap (configData);

	associatedJoinWP = tempConfig.get_JoinWP();
	lhsSlotPairContainer.swap(tempConfig.get_columnsToSlotsPairLhs());
	rhsSlotPairContainer.swap(tempConfig.get_columnsToSlotsPairRhs());

	// check if the file scanner is started, if not contact the diskPool and start it
	// make sure the internal datastructures get initialized at this time
	if (!fileIdRHS.IsValid() || !fileIdLHS.IsValid()){
		// start file, some temporary relation name need to be sent from top
		myLHSName = tempConfig.get_lhsrelName();
		myRHSName = tempConfig.get_rhsrelName();
		int _numColsLHS = lhsSlotPairContainer.Length();
/*
#ifdef DEBUG
		lhsSlotPairContainer.MoveToStart();
		while (lhsSlotPairContainer.RightLength()) {
			lhsSlotPairContainer.Current().Print();
			lhsSlotPairContainer.Advance();
		}
#endif
*/
		//FOREACH_EM(key, data, tempConfig.get_columnsToSlotsMapLhs()){
		//	_numColsLHS++;
		//}END_FOREACH;

		int _numColsRHS = rhsSlotPairContainer.Length();
/*
#ifdef DEBUG
		rhsSlotPairContainer.MoveToStart();
		while (rhsSlotPairContainer.RightLength()) {
			rhsSlotPairContainer.Current().Print();
			rhsSlotPairContainer.Advance();
		}
#endif
*/
		//FOREACH_EM(key, data, tempConfig.get_columnsToSlotsMapRhs()){
		//	_numColsRHS++;
		//}END_FOREACH;

		fileIdLHS = globalDiskPool.AddFile(myLHSName.c_str(), _numColsLHS);
		fileIdRHS = globalDiskPool.AddFile(myRHSName.c_str(), _numColsRHS);
	}
	GetFlowThruQueryExits (myQueryExits);
	//	assert(myQueryExits.Length());

	// Fill the myQueryExits
	//QueryExitContainer& queries = tempConfig.get_QE();
	//tempConfig.get_flowThroughQueryExits().MoveToStart ();
	//myQueryExits.MoveToFinish ();
	//assert(tempConfig.get_flowThroughQueryExits().Length());
	//myQueryExits.SwapRights (tempConfig.get_flowThroughQueryExits());
	//assert(myQueryExits.Length());

	/*
	QueryExitContainer delQueriesLhs; // blank just to suffice interface
	// tell the column manager its part of the config
	colManagerLHS.ChangeMapping(tempConfig.get_queryColumnsMapLhs(),
				    tempConfig.get_columnsToSlotsMapLhs(),
				    delQueriesLhs); // delQueries is blank for us

	QueryExitContainer delQueriesRhs; // blank just to suffice interface
	// tell the column manager its part of the config
	colManagerRHS.ChangeMapping(tempConfig.get_queryColumnsMapRhs(),
				    tempConfig.get_columnsToSlotsMapRhs(),
				    delQueriesRhs); // delQueries is blank for us
*/
}

void TileJoinWayPointImp::GenerateTokenRequests(){

	// This limit should be CPU limit, but let it be same for now
	//for (; numDiskRequestsOut < FILE_SCANNER_MAX_NO_CHUNKS_REQUEST; numDiskRequestsOut++) {
	for (; numDiskRequestsOut < 8; numDiskRequestsOut++) {
		RequestTokenNowDelayOK (DiskWorkToken::type);
	}
}

// We can only read only one chunk at a time. We need a tile to work on so keep track of what has been read so far
// First go through LHS chunks to be read and then go through RHS chunks, but we can only send one read request, then token is lost
// once we send read/write request. This function is only inner loop of tile processing in a way.
//
// We need a tile to have IO on and a tile to have processing ON
// We should not send all the tilers for processing at once, should limit the toal number to some constant or track kill
// messages from the merger (better solution probably)
void TileJoinWayPointImp :: RequestGranted (GenericWorkToken &returnVal) {

	// note that we have one less request out
        if (CHECK_DATA_TYPE(returnVal, DiskWorkToken)) {
		ScheduleIO(returnVal);

        }
}

void TileJoinWayPointImp :: ScheduleIO(GenericWorkToken &returnVal) {

	PDEBUG("TileJoinWayPointImp :: ScheduleIO");
	DiskWorkToken myToken;
	myToken.swap (returnVal);
	//numDiskRequestsOut--;

	// If IO floor is valid and read completely, given token back and return
	if(workFloor.IsValid() && LHSListIO.Length() == workFloor.LHSSize() && RHSListIO.Length() == workFloor.RHSSize()) {
		// CPU floor empty means CPU work is done and we are ready to swap it with IO floor to start work again
		if (LHSListCPU.IsEmpty()) {
			// If CPU floor is empty, swap IO floor with CPU floor and start CPU work and IO work
			assert(RHSListCPU.IsEmpty());
			LHSListCPU.swap(LHSListIO);
			RHSListCPU.swap(RHSListIO);
			workFloor.Clear();
			CPUBucket = IOBucket;
			IOBucket = -1;
			// Launch the work
			ScheduleWork();
		} else {
			GiveBackToken(myToken);
			numDiskRequestsOut--;
			return;
		}
	}

	// Read IO  slice to get chunk IDs once
	if (!workFloor.IsValid()) {
		__uint64_t bucket;
		vector<ChunkID> veclhsID;
		vector<ChunkID> vecrhsID;
		// We just get the slice here once and then we just extract chunk ID from it which
		// we are supposed to read. Once we take out chunk IDs, return the slice back because
		// ScheduleWork will take them out again (all of them from one bucket) one by one and
		// work on it.
		if (!meta.GetSlice(bucket, veclhsID, vecrhsID)) {
			// if there is no slice left, we are done reading
			GiveBackToken(myToken);
			return; // dont ask for more tokens if we are done reading all floors
		}
		// Give back slice as we will read it later while CPU work
		meta.UnprocessedSlice(bucket, veclhsID, vecrhsID);
		// These floors are just metadata, not actual chunks
		ioFloor.Set(bucket, veclhsID, vecrhsID);
		workFloor.Set(bucket, veclhsID, vecrhsID);
		// actual bucket lies in MSB
		IOBucket = bucket>>32;
	}

	// Start reading IO floor if not done yet
	ChunkID lhsID;
	ChunkID rhsID;
	__uint64_t bucket;
	bool found = false;
	bool isLHS = false;
	SlotPairContainer tmp;
	if (ioFloor.GetNextLHS(bucket, lhsID)) {
		found = true;
		isLHS = true;
		tmp.copy(lhsSlotPairContainer);
	} else if (ioFloor.GetNextRHS(bucket, rhsID)) {
		found = true;
		isLHS = false;
		tmp.copy(rhsSlotPairContainer);
	}

	// If any of LHS or RHS chunkID is still not read, read it
	if (found) {
		QueryExitContainer myOutputExitsCopy;
		myOutputExitsCopy.copy (myQueryExits);
		// create history which we will use
		WayPointID temp1 = GetID();
		TileJoinScanHistory myHistory (temp1, bucket, lhsID, isLHS, string(""), myOutputExitsCopy); // TBD, improve file
		HistoryList lineage;
		lineage.Insert (myHistory);
		QueryExitContainer myOutputExitsCopy2;
		myOutputExitsCopy2.copy (myQueryExits);
		ChunkID chunkIDcopy = lhsID;
		if (!isLHS)
			chunkIDcopy = rhsID;
		WayPointID temp2 = GetID();
		globalDiskPool.ReadRequest(chunkIDcopy, temp2, true/*use uncompressed*/, lineage, myOutputExitsCopy2, myToken, tmp);

	} else {
		GiveBackToken(myToken);
		numDiskRequestsOut--;
	}
}

void TileJoinWayPointImp :: ScheduleWork() {

	PDEBUG("TileJoinWayPointImp :: ScheduleWork");

	// If CPU floor is empty, return
	if (LHSListCPU.IsEmpty()) {
		assert(RHSListCPU.IsEmpty());
		return;
	}

	// Just in case we don't go in infinite loop
	int sanityCount = 0;

	//meta.PrintAllSlices();

	// Continue this loop, until we read all slice from one bucket (as soon as bucket number changes, break)
	// Or we are done with all slices (no more slices left)
	while (1) {

		// Wes should not have that many guys, but in theory it's OK to have as many as we want
		// and in that case, remove this assert
		assert (sanityCount <= 1024);
		// Get the slice
		__uint64_t bucket = 0;
		vector<ChunkID> lhsList;
		vector<ChunkID> rhsList;
		// Get each slice and work on it
		if (!meta.GetSlice(bucket, lhsList, rhsList)) {
			// If no slice found, get out
			LHSListCPU.Clear();
			RHSListCPU.Clear();
			break;
		}

		int start = lhsList[0].GetFragmentStart();
		int end = lhsList[0].GetFragmentEnd();
		// If done with one floor, dont process new floor becoz it's not IO'ed yet
		// this is made sure by comparing with actual bucket which is IO'ed
		if (CPUBucket != (bucket>>32)) {
			// return the slice back to process it again later
			meta.UnprocessedSlice(bucket, lhsList, rhsList);
			lhsList.clear();
			rhsList.clear();
			LHSListCPU.Clear();
			RHSListCPU.Clear();
			break;
		}

		// Get the immediate token
		GenericWorkToken returnVal;
		if (!RequestTokenImmediate (CPUWorkToken::type, returnVal, 1)) {
			// If token not received, return the slice back to process it later
			meta.UnprocessedSlice(bucket, lhsList, rhsList);
			// and get out
			break;
		}

		FATALIF(returnVal.Type() != CPUWorkToken::type, "We got a fake token");

		// sanity
		assert(!lhsList.empty());
		assert(!rhsList.empty());
		assert(LHSListCPU.Length());
		assert(RHSListCPU.Length());

		// get query exits
		QueryExitContainer myDestinations;
		myDestinations.copy (myQueryExits);
		// get waypoint ID
		WayPointID temp1 = GetID();

		// get actual chunks copies
		ContainerOfChunks LHSListCopy;
		LHSListCopy.copy(LHSListCPU);
		ContainerOfChunks RHSListCopy;
		RHSListCopy.copy(RHSListCPU);

/*
 		This is just to debug number of tuples

		LHSListCopy.MoveToStart();
		while (LHSListCopy.RightLength()) {
			BStringIterator biter;
			LHSListCopy.Current().SwapBitmap(biter);
			TOTALLHS += biter.GetNumTuples();
			LHSListCopy.Current().SwapBitmap(biter);
			LHSListCopy.Advance();
		}

		RHSListCopy.MoveToStart();
		while (RHSListCopy.RightLength()) {
			BStringIterator biter;
			RHSListCopy.Current().SwapBitmap(biter);
			TOTALRHS += biter.GetNumTuples();
			RHSListCopy.Current().SwapBitmap(biter);
			RHSListCopy.Advance();
		}
*/

		//TOTALLHS++;
		//printf("\n Num slices = %d", TOTALLHS); fflush(stdout);

		// Create work description
		JoinMergeWorkDescription workDesc (temp1, start, end, myDestinations, LHSListCopy, RHSListCopy);

		WorkFunc myFunc;
		myFunc = GetWorkFunction (JoinMergeWorkFunc::type);
		QueryExitContainer myDestinations2;
		myDestinations2.copy (myQueryExits);
		HistoryList lineage; // TBD, blank for now
		QueryExitContainer myOutputExitsCopy;
		myOutputExitsCopy.copy (myQueryExits);

		// create chunkIDs for history
		ChunkIDContainer contLHS;
		for (int i = 0; i < lhsList.size(); i++) {
			assert(lhsList[i].GetFragmentStart() == start);
			assert(lhsList[i].GetFragmentEnd() == end);
			contLHS.MoveToFinish();
			contLHS.Insert(lhsList[i]);
		}
		ChunkIDContainer contRHS;
		for (int i = 0; i < rhsList.size(); i++) {
			assert(rhsList[i].GetFragmentStart() == start);
			assert(rhsList[i].GetFragmentEnd() == end);
			contRHS.MoveToFinish();
			contRHS.Insert(rhsList[i]);
		}

		// create history
		TileJoinMergeHistory mHistory(GetID(), bucket, contLHS, contRHS,  myOutputExitsCopy);
		lineage.Append(mHistory);
		WayPointID temp2 = GetID();

		//printf("\nLaunched slice %d, (%d,%d)", sanityCount++, start, end);
		// Launch the actual work
		myCPUWorkers.DoSomeWork (temp2, lineage, myDestinations2, returnVal, workDesc, myFunc);

	}
}

// As per Chris, we dont get StartProducing, just start by default as soon as writing is done
// so basically we dont need this function. Remove it if not useful
void TileJoinWayPointImp :: ProcessHoppingUpstreamMsg (HoppingUpstreamMsg &message) {

/*
	Notification& msg = message.get_msg ();

	// We will be getting writing message first for all LHS and RHS chunks before we start producing them
	if (msg.Type() == StartWritingMsg::type) {

		// Get the message
		StartWritingMsg myMessage;
		msg.swap (myMessage);
	}
	else {
		FATAL( 	"Strange, why did a table scan get a HUS of a type that was not 'Start Writing'?");
	}

	// make sure we get to do some of the work we need
	GenerateTokenRequests();
*/
}

/**
		For tiles, we will get this for tile IDs via lineage
*/
void TileJoinWayPointImp :: ProcessDropMsg (QueryExitContainer &whichExits,
																						 HistoryList &lineage) {

	PDEBUG("TileJoinWayPointImp :: ProcessDropMsg");
	// make sure that the HistoryList has one item that is of the right type
	lineage.MoveToStart ();

	// get the history out
	if (lineage.Current ().Type()== TileJoinScanHistory::type) {
		TileJoinScanHistory myHistory;
		lineage.Remove (myHistory);

		ChunkID cnkID = myHistory.get_whichChunk ();
		int bucketID = myHistory.get_bucketID();
	} else if (lineage.Current ().Type()== TileJoinMergeHistory::type) {
		TileJoinMergeHistory myHistory;
		lineage.Remove (myHistory);
		ChunkIDContainer &lhsList = myHistory.get_chunkIDLHSList();
		ChunkIDContainer &rhsList = myHistory.get_chunkIDLHSList();
		__uint64_t bucket = myHistory.get_bucketID();
		vector<ChunkID> vecLhs;
		lhsList.MoveToStart();
		while (lhsList.RightLength()) {
			vecLhs.push_back(lhsList.Current());
			lhsList.Advance();
		}
		vector<ChunkID> vecRhs;
		rhsList.MoveToStart();
		while (rhsList.RightLength()) {
			vecRhs.push_back(rhsList.Current());
			rhsList.Advance();
		}
		meta.UnprocessedSlice(bucket, vecLhs, vecRhs);
		printf("\n DROPPED ");
	}

	GenerateTokenRequests();
}

/**
		This is positive acknowledgement for tiles, (bucket, lhsChunkID, rhsChunkID)
*/
void TileJoinWayPointImp :: ProcessAckMsg (QueryExitContainer &whichExits, HistoryList &lineage) {

	PDEBUG("TileJoinWayPointImp :: ProcessAckMsg");
	// make sure that the HistoryList has one item that is of the right type
	lineage.MoveToStart ();
	FATALIF (lineage.RightLength () != 1, "Got a bad lineage item in an ack to a tile join scan waypoint!");

	if (lineage.Current ().Type() == TileJoinScanHistory::type) {
		// get the history out
		TileJoinScanHistory myHistory;
		lineage.Remove (myHistory);
	} else if (lineage.Current ().Type() == TileJoinWriteHistory::type) {
		TileJoinWriteHistory myHistory;
		lineage.Remove (myHistory);
	} else if (lineage.Current ().Type() == TileJoinMergeHistory::type) {
		TileJoinMergeHistory myHistory;
		lineage.Remove (myHistory);
		number128++;
		printf("\nMerged fragment received ..... %d   ", number128); fflush(stdout);
		int numDivisions = meta.GetNumFragments() / NUM_FRAGMENTS_PER_SLICE;
		if (meta.GetNumFragments()%NUM_FRAGMENTS_PER_SLICE!= 0)
			numDivisions++;
		if (number128 == numDivisions*NUM_SEGS) {
			QueryExitContainer allCompleteCopy;
			allCompleteCopy.copy (myQueryExits);
			QueryDoneMsg someAreDone (GetID (), myQueryExits);
			HoppingDownstreamMsg myOutMsg (GetID (), allCompleteCopy, someAreDone);
			SendHoppingDownstreamMsg (myOutMsg);
		}
		ScheduleWork();
	} else {
		FATALIF(true, "Got a bad lineage item in an ack to a tile join scan waypoint!");
	}

	// This is what is needed here
	GenerateTokenRequests();
}

// this message is coming to write chunks
// This receives disk token in message itself (look upstream message sent from HashCleaner)
// ExtractionContainer has tokens inside, use those
// You need a different token for each Write request. Then look where we scan ExtractionContainer should pull a token from tokenlist
void TileJoinWayPointImp :: ProcessHoppingDataMsg (HoppingDataMsg &data){

	PDEBUG("TileJoinWayPointImp :: ProcessHoppingDataMsg");
	// convert the token into the correct type
	DiskWorkToken myToken[10];
	// myToken.swap (returnVal);

	// create the work spec and get it done!
	QueryExitContainer myDestinations;
	myDestinations.copy (data.get_dest ());

	// extract the chunk from the message
	ExtractionContainer chunkCont;
	//data.get_data ().swap (chunkCont);
	chunkCont.swap (data.get_data ());

	int bucketID = chunkCont.get_whichSegment();
	// Get the two way list
	ExtractionList extractResult;
	extractResult.swap (chunkCont.get_result());

	// iterate and extract join waypoint chunks
	extractResult.MoveToStart();

	assert(extractResult.Length() != 0);

	int i = 0;
	while (i < extractResult.Length()) {
		GenericWorkToken returnVal;
		if (!RequestTokenImmediate (DiskWorkToken::type, returnVal, 1)) { // high priority
			SendDropMsg (data.get_dest (), data.get_lineage ());
			for (int j = 0; j < i; j++)
				GiveBackToken (myToken[j]);
			return;
		}
		// convert the token into the correct type
		myToken[i].swap (returnVal);
		i++;
	}
	i = 0;
	while (extractResult.RightLength()) {

		ExtractedChunk& chunkToProcess = extractResult.Current();

			// Our waypoint ID
			WayPointID tempID = GetID ();

			// Set the number of fragments
			Column hcol;
			chunkToProcess.get_myChunk().SwapHash(hcol);
			meta.SetNumFragments(hcol.GetFragments().GetNumFragments());
			chunkToProcess.get_myChunk().SwapHash(hcol);

			if (chunkToProcess.get_isLHS() != 0) { // LHS chunk

				off_t _chunkId = numChunksLHS;
				numChunksLHS++;
				// send the LHS request
				ChunkID chunkIDLHS(_chunkId, fileIdLHS);
				// inform the tile metadata manager
				meta.ChunkToDisk (bucketID, chunkIDLHS, true);
				SlotPairContainer colsToProcess;
				colsToProcess.copy(lhsSlotPairContainer);
				HistoryList hList;
				TileJoinWriteHistory wHistory(GetID());
				hList.Append(wHistory);
				LOG_ENTRY_P(2, "LHS CHUNK %d of %s sent for WRITING with NumTuples %d",
						_chunkId, myLHSName.c_str(), chunkToProcess.get_myChunk().GetNumTuples()) ;
				// send the request
				globalDiskPool.WriteRequest(chunkIDLHS, tempID, chunkToProcess.get_myChunk(), hList/*data.get_lineage()*/,
						myDestinations, myToken[i], colsToProcess);
				i++;


			} else { // RHS chunk

				off_t _chunkId = numChunksRHS;
				numChunksRHS++;
				// send the RHS request
				ChunkID chunkIDRHS(_chunkId, fileIdRHS);
				// inform the tile metadata manager
				meta.ChunkToDisk (bucketID, chunkIDRHS, false);
				SlotPairContainer colsToProcess;
				colsToProcess.copy(rhsSlotPairContainer);
				HistoryList hList;
				TileJoinWriteHistory wHistory(GetID());
				hList.Append(wHistory);
				LOG_ENTRY_P(2, "RHS CHUNK %d of %s sent for WRITING with NumTuples %d",
						_chunkId, myRHSName.c_str(), chunkToProcess.get_myChunk().GetNumTuples()) ;
				// send the request
				globalDiskPool.WriteRequest(chunkIDRHS, tempID, chunkToProcess.get_myChunk(), hList/*data.get_lineage()*/,
						myDestinations, myToken[i], colsToProcess);
				i++;

			}
		//} // if join WP
		extractResult.Advance();
	} // while

	// Send ACK to the cleaner with its only history lineage
	SendAckMsg (data.get_dest (), data.get_lineage ());
}

void TileJoinWayPointImp :: ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message) {

	PDEBUG("TileJoinWayPointImp :: ProcessHoppingDownstreamMsg");
	// see if we have a query done message
	if (CHECK_DATA_TYPE (message.get_msg (), QueryDoneMsg)) {

	  LOG_ENTRY_P(2, "Starting Mergin phase in %s", "me");
		///globalDiskPool.Flush(fileIdLHS);
		//globalDiskPool.Flush(fileIdRHS);
		// Fire up reading
		GenerateTokenRequests();

	} else {
		FATAL ("Why did I get some hopping downstream message that was not a query done message?\n");
	}
}

// Three guys can send done producing, one is cleaner in which case our history type will be only one
// guy, HashCleanerHistory::type
// And we will also receive it once we start reading in which case our history will contain one guy
// TileJoinScanHistory::type
// We also get this when we send write request to disk from ProcessHoppingDataMsg, TileJoinWriteHistory probably
void TileJoinWayPointImp :: DoneProducing (QueryExitContainer &whichOnes, HistoryList &history,
		int result, ExecEngineData& data) {

	PDEBUG("TileJoinWayPointImp :: DoneProducing");
	history.MoveToStart();
	while (history.RightLength()) {
		history.Advance();
	}
	history.MoveToStart();
	FATALIF (history.RightLength () == 0, "Got no lineage item in an ack to a tile join scan waypoint!");
	//FATALIF (history.Current ().Type() != HashCleanerHistory::type, "Got a bad lineage item in an ack to a tile join scan waypoint!");

	history.MoveToStart();
	switch (history.Current ().Type()){

		case TileJoinScanHistory::type:
			{
				numDiskRequestsOut--;
				// get the history out
				TileJoinScanHistory myHistory;
				history.Remove (myHistory);

				ChunkContainer temp;
				data.swap (temp);

				Column col;
				temp.get_myChunk().SwapColumn (col, BITSTRING_SLOT);
				assert(col.IsValid());
				temp.get_myChunk().CreateBitstringFromCol(col);

				ChunkID tempID = myHistory.get_whichChunk();
				// These two lists acts as one to one mapping between ChunkID and Chunk
				if (myHistory.get_isLHS()) {
					LHSListIO.Append (temp.get_myChunk());
				} else {
					RHSListIO.Append (temp.get_myChunk());
				}

				GenerateTokenRequests();
			}
			break;

		case TileJoinWriteHistory::type:
			// update the number of requests out to know when we are down writing
			// on the disk
			break;

		case TileJoinMergeHistory::type:
			numCPURequestsOut--;
			//SendAckMsg (whichOnes, history);
			GenerateTokenRequests();
			break;
	}
}
