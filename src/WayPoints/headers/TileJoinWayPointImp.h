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
#ifndef TILEJOIN_IMP_H
#define TILEJOIN_IMP_H

#include "WayPointImp.h"
#include "TableScanHelpers.h"
#include "Bitmap.h"
#include "ID.h"
#include "EfficientMap.h"
#include "TileManager.h"
#include <map>
#include <vector>
#include "Swap.h"
#include "DistributedEfficientMap.cc"

struct FloorOfChunks {
    typedef std::vector<ChunkID> ChunkVector;

    __uint64_t bucket;
    ChunkVector chunksLHS;
    ChunkVector chunksRHS;

    void swap (FloorOfChunks& swapMe) {
        SWAP_STD(bucket, swapMe.bucket);
        chunksLHS.swap(swapMe.chunksLHS);
        chunksRHS.swap(swapMe.chunksRHS);
    }

    bool IsValid() {return !(chunksLHS.empty() && chunksRHS.empty());}

    void Set(__uint64_t _bucket, ChunkVector& _chunksLHS, ChunkVector& _chunksRHS) {
        bucket = _bucket;
        chunksLHS = _chunksLHS;
        chunksRHS = _chunksRHS;
    }

    bool GetNextLHS(__uint64_t& _bucket, ChunkID& rez) {
        if (chunksLHS.empty())
            return false;
        _bucket = bucket;
        rez = chunksLHS[0];
        chunksLHS.erase(chunksLHS.begin());
        return true;
    }

    bool GetNextRHS(__uint64_t& _bucket, ChunkID& rez) {
        if (chunksRHS.empty())
            return false;
        _bucket = bucket;
        rez = chunksRHS[0];
        chunksRHS.erase(chunksRHS.begin());
        return true;
    }

    __uint64_t GetBucket() {return bucket;}

    void Clear () {
        bucket = -1;
        chunksLHS.clear();
        chunksRHS.clear();
    }

    int LHSSize() {return chunksLHS.size();}
    int RHSSize() {return chunksRHS.size();}

};

class TileJoinWayPointImp : public WayPointImp {
    private:

        enum TileJoinModes {
            cTileJoinRead,
            cTileJoinWrite
        };

        // This is meta data manager and tile producer
        ChunkMetaData meta;

        //id of the FileScanner object which reads LHS part
        TableScanID fileIdLHS;

        //id of the FileScanner object which reads RHS part
        TableScanID fileIdRHS;

        WayPointID associatedJoinWP;

        // names
        std::string myLHSName;
        std::string myRHSName;

        // manager of columns for each Query Exit
        ColumnManager colManagerLHS;
        ColumnManager colManagerRHS;

        // maintain these counts while writing chunks
        int LHSChunkCount;

        int RHSChunkCount;

        // monitor number of token requests out to make sure we are as aggressive as we can
        int numCPURequestsOut;
        int numDiskRequestsOut;

        // Used to generate unique ID for LHS and RHS chunks using fileID
        off_t numChunksLHS;
        off_t numChunksRHS;

        int TOTALLHS;
        int TOTALRHS;

        // This keeps static set of queries to be sent with chunk
        QueryExitContainer myQueryExits;

        // Which mode I am in,
        // unconfigured,
        // configured and ready to write,
        // done writing and ready to read,
        TileJoinModes myMode;

        FloorOfChunks ioFloor;
        FloorOfChunks workFloor;
        int number128;
        ContainerOfChunks LHSListIO;
        ContainerOfChunks RHSListIO;
        ContainerOfChunks LHSListCPU;
        ContainerOfChunks RHSListCPU;
        int IOBucket;
        int CPUBucket;

        SlotPairContainer lhsSlotPairContainer;
        SlotPairContainer rhsSlotPairContainer;

    private:

        // funtion to keep a constant suply of write tokens so we can do agressive IO
        void GenerateTokenRequests();

        // function to find a chunk that needs to be generated
        // returns "false" if no such chunk exists
        bool ChunkRequestIsPossible(size_t &_chunkId);

        void ScheduleIO(GenericWorkToken &returnVal);

        void ScheduleWork();


    public:

        // constructor and destructor
        TileJoinWayPointImp();
        ~TileJoinWayPointImp();

        // here we over-ride the standard WayPointImp functions
        void TypeSpecificConfigure (WayPointConfigureData &configData);
        void RequestGranted (GenericWorkToken &returnVal);
        void ProcessHoppingUpstreamMsg (HoppingUpstreamMsg &message);
        // downstream is received to make sure all writing is done from join cleaner
        void ProcessHoppingDownstreamMsg (HoppingDownstreamMsg &message);
        void ProcessDropMsg (QueryExitContainer &whichOnes, HistoryList &lineage);
        void ProcessAckMsg (QueryExitContainer &whichOnes, HistoryList &lineage);
        void ProcessHoppingDataMsg (HoppingDataMsg &data);
        void DoneProducing (QueryExitContainer &whichOnes, HistoryList &history, int result, ExecEngineData& data);

};

#endif
