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
#ifndef TABLE_IMP_H
#define TABLE_IMP_H

#include "WayPointImp.h"
#include "TableScanHelpers.h"
#include "ID.h"
#include "EfficientMap.h"
#include "DiskPool.h"

#include <string>

class TableWayPointImp : public WayPointImp {
    private:

        typedef DiskPool::ClusterRange ClusterRange;
        typedef DiskPool::ClusterRangeList ClusterRangeList;

        //id of the FileScanner object
        TableScanID fileId;

        // name so we do not have to keep on looking it up
        std::string myName;

        //bitmap for chunks and queries --> a BITSTRING for each chunk indicating what queries was the chunk delievered for
        //Bitmap* queryChunkMap;
        QueryChunkMap* queryChunkMap;

        // bitmap with all the acknowledged chunks
        // we use this to enforce consistency
        QueryChunkMap* ackQueries;

        // the manager for the efficient QueryExit to Bitstring mapper
        QEToBitstring qeTranslator;

        // manager of columns for each Query Exit
        ColumnManager colManager;

        // keep track of the number of chunks acknowledged/queryExit to know when we are done
        typedef std::map<QueryExit, int> QECounters;
        QECounters qeCounters;

        // queries that are done. Need to keep track of this to
        // hunt for rogue messages that come after queries are done
        Bitstring doneQueries;

        // monitor number of token requests out to make sure we are as aggressive as we can
        int numRequestsOut;

        // last chunk we generated to ensure a circular list behavior
        int lastChunkId;

        // number of chunks; used mostly to know what chunkID to generate
        // for  write
        off_t numChunks;

        ClusterRangeList clusterRanges;

        QueryToScannerRangeList queryClusterRanges;

        /// AUXILIARY FUNCTIONS
        // look for queries that can tag chunk _chunkId
        Bitstring FindQueries(off_t _chunkId);
        // funtion to keep a constant suply of write tokens so we can do agressive IO
        void GenerateTokenRequests();
        // function to find a chunk that needs to be generated
        // returns "false" if no such chunk exists
        bool ChunkRequestIsPossible(off_t &_chunkId);

        void AcknowledgeChunk(int chunkID, QueryIDSet queries);


    public:

        // constructor and destructor
        TableWayPointImp();
        ~TableWayPointImp();

        // here we over-ride the standard WayPointImp functions
        void TypeSpecificConfigure (WayPointConfigureData &configData);
        void RequestGranted (GenericWorkToken &returnVal);
        void ProcessHoppingUpstreamMsg (HoppingUpstreamMsg &message);
        void ProcessDropMsg (QueryExitContainer &whichOnes, HistoryList &lineage);
        void ProcessAckMsg (QueryExitContainer &whichOnes, HistoryList &lineage);
        void ProcessHoppingDataMsg (HoppingDataMsg &data);
        void DoneProducing(QueryExitContainer&, HistoryList&, int, ExecEngineData&);
        void ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message);
        void Debugg(void);
};

#endif
