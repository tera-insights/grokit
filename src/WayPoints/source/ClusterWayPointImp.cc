//
//  Copyright 2014 Tera Insights, LLC
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

#include "ClusterWayPointImp.h"
#include "DiskPool.h"
#include "WPFExitCodes.h"
#include "CPUWorkerPool.h"
#include "EEExternMessages.h"
#include "Errors.h"
#include "EventProcessor.h"

extern EventProcessor globalCoordinator;

ClusterWayPointImp::ClusterWayPointImp():
	WayPointImp(),
	relation()
{ }

ClusterWayPointImp::~ClusterWayPointImp() {

}

void ClusterWayPointImp::SendFlushRequest() {
	TableScanID scanID = TableScanID::GetIdByName(relation.c_str());

	globalDiskPool.Flush(scanID);

	QueryExitContainer endingExits;
	GetEndingQueryExits(endingExits);

	// Send a message to the coordinator that we are done
	QueriesDoneMessage::Factory(globalCoordinator, endingExits);
}

void ClusterWayPointImp::TypeSpecificConfigure(WayPointConfigureData& configData) {
	ClusterConfigureData config;
	config.swap(configData);

	relation = config.get_relation();

	QueryExitContainer endingExits;
	GetEndingQueryExits(endingExits);

	FOREACH_TWL(qe, endingExits) {
		SendStartProducingMsg(qe);
	} END_FOREACH;
}

void ClusterWayPointImp::ProcessHoppingDownstreamMsg(HoppingDownstreamMsg& message) {
	if(CHECK_DATA_TYPE(message.get_msg(), QueryDoneMsg)) {
		SendFlushRequest();
	}
}

void ClusterWayPointImp::DoneProducing(
	QueryExitContainer& whichOnes,
	HistoryList &history,
	int result,
	ExecEngineData& data)
{
	PDEBUG("ClusterWayPointImp :: DoneProducing()");

	FATALIF(result != WP_PROCESS_CHUNK,
		"Got invalid result type from work function in ClusterWayPointImp");

	ClusterProcessChunkRez myData;
	myData.swap(data);

	DiskPool::ClusterRange range(myData.get_min(), myData.get_max());

	// Get ChunkID from history and send an update for the cluster range
	// We only allow the history to contain exactly one item from a
	// table reader
	EXTRACT_HISTORY_ONLY(history, histItem, TableReadHistory);
	ChunkID chunkID = histItem.get_whichChunk();
	PUTBACK_HISTORY(history, histItem);

	WayPointID myID = GetID();
	globalDiskPool.UpdateClusterRange(chunkID, myID, range);

	// Acknowledge the chunk
	SendAckMsg(whichOnes, history);
}

void ClusterWayPointImp::ProcessHoppingDataMsg(HoppingDataMsg& data) {
	PDEBUG("ClusterWayPointImp :: ProcessHoppingDataMsg");

	// Request a work token to actually run the clustering
	GenericWorkToken returnVal;
	if( !RequestTokenImmediate(CPUWorkToken::type, returnVal) ) {
		// Didn't get one, drop the chunk
		SendDropMsg(data.get_dest(), data.get_lineage());
		return;
	}

	// Convert token to the correct type
	CPUWorkToken myToken;
	myToken.swap(returnVal);

	// Extract the chunk
	ChunkContainer temp;
	data.get_data().swap(temp);
	Chunk& chunk = temp.get_myChunk();

	// Create the work spec
	QueryExitContainer whichOnes;
	whichOnes.copy(data.get_dest());

	ClusterProcessChunkWD workDesc(chunk);

	WayPointID myID = GetID();
	WorkFunc myFunc = GetWorkFunction(ClusterProcessChunkWorkFunc::type);
	myCPUWorkers.DoSomeWork(myID,
		data.get_lineage(), whichOnes, myToken, workDesc, myFunc);
}