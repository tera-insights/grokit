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
#include "WayPointImp.h"
#include "TextLoaderInternal.h"
#include "ID.h"

#ifndef TEXT_LOADER_IMP_H
#define TEXT_LOADER_IMP_H

/** Waypoint that reads plain text and builds chunks.

Capabilities:
1. Can work in parallel from many files
2. Aggressive so that a lot of work is done simultaneously
3. Can skip columns (addapts to the query plan)

Limitations:
1. Cannot support dropped chunks for now
This is due to the fact hat pipes cannot be rewind so a
complicated dance involving caching chunks needs to be used (a
chunk is let go only when a confirmation of it's processing is
obtained). This seems to be unavoidable with pipes.

*/

class TextLoaderWayPointImp : public WayPointImp {

    private:

        // list of tasks we can accomplish
        TextLoaderDSContainer tasks;

        // these are the query-exits we are producing data for
        QueryExitContainer myExits;

        // how many streams we have overall
        // when a stream is closed, we decrease this counter
        // when the counter is 0 and all the chunks are back we know we are done
        off_t numStreams;

        // how many tokens we asked for so far
        off_t tokensRequested;

        // chunks out for writting
        off_t chunksOut;

        // counte for requests
        // in the future it will be used to tell chunks apart
        off_t requestCnt;

        // the scan id we created for ourselves to tag our chunks
        TableScanID tID;

        // my name, for  fast access
        std::string name;

        ///// BEGIN TEMPORARY SOLUTION /////
        // System time we last sent out a cached chunk. Used for throttling.
        double lastCacheSend;

        // Number of seconds inbetween sent chunks when caching
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

    public:

        // constructor and destructor
        TextLoaderWayPointImp();
        ~TextLoaderWayPointImp();

        // here we over-ride the standard WayPointImp functions
        void TypeSpecificConfigure (WayPointConfigureData &configData);
        void RequestGranted (GenericWorkToken &returnVal);
        void ProcessHoppingUpstreamMsg (HoppingUpstreamMsg &message);
        void ProcessDropMsg (QueryExitContainer &whichOnes, HistoryList &lineage);
        void DoneProducing (QueryExitContainer &whichOnes, HistoryList &history,
                int returnVal, ExecEngineData& data);
        void ProcessAckMsg (QueryExitContainer &whichOnes, HistoryList &lineage);

};

#endif
