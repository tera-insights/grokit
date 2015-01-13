//
//  Copyright 2013 Christopher Dudley
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

#ifndef GI_WAYPOINT_IMP_H
#define GI_WAYPOINT_IMP_H

#include "WayPointImp.h"
#include "GI_Internal.h"
#include "EfficientMap.h"
#include "ExecEngineData.h"
#include "TableScanID.h"

class GIWayPointImp : public WayPointImp {

    // Mapping of open streams
    typedef Keyify<off_t> StreamKey;
    typedef EfficientMap<StreamKey, GIStreamInfo> GIStreamMap;
    GIStreamMap open_streams;

    // List of files used by this waypoint
    StringContainer my_files;

    // List of open stream proxies and their corresponding states.
    GITaskList tasks;

    // The query exits we are producing data for
    QueryExitContainer myExits;

    // The number of open streams we have
    size_t num_open_streams;

    // The number of un-acked chunks we have sent out.
    size_t num_chunks_out;

    // The ID number for the next chunk to produce.
    // Also counts as the number of chunks produced thus far.
    size_t next_chunk_no;

    // The number of tokens we have asked for so far
    size_t tokensRequested;

    // the scan id we created for ourselves to tag our chunks
    TableScanID tID;

    ///// BEGIN TEMPORARY SOLUTION /////
    // System time we last sent out a cached chunk. Used for throttling.
    double last_cache_send;

    // Number of seconds in between sent chunks when caching
    static const double CACHE_INTERVAL;
    ///// END TEMPORARY SOLUTION /////

    // Shallow copies of chunks sent out
    typedef EfficientMap<ChunkID, ChunkContainer> ChunkMap;
    ChunkMap chunkMap;

    // Cache of chunks that need to be sent out
    typedef TwoWayList<CachedChunk> ChunkCache;
    ChunkCache chunkCache;

    // function to request as many tokens as we could use
    void RequestTokens();

    // function to send out a chunk (used for sending chunks from cache).
    void SendCachedChunk( CachedChunk& chunk );

    // function to set up the streams
    void SetUpStreams();

public:

    // Constructor and destructor
    GIWayPointImp();
    ~GIWayPointImp();

    // Overrides for standard WayPointImp methods
    void TypeSpecificConfigure( WayPointConfigureData &configure );
    void RequestGranted( GenericWorkToken &returnVal );
    void ProcessHoppingUpstreamMsg( HoppingUpstreamMsg &message );
    void ProcessDropMsg( QueryExitContainer &whichOnes, HistoryList &history );
    void DoneProducing( QueryExitContainer &whichOnes, HistoryList &history,
            int returnVal, ExecEngineData & data );
    void ProcessAckMsg( QueryExitContainer &whichOnes, HistoryList &lineage );
};

#endif // GI_WAYPOINT_IMP_H
